#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "ReplayScript.hpp"
#include "Constants.hpp"
#include "GitInfo.h"
#include "Impacts.hpp"

#include "TrackData.hpp"
#include "ReplayData.hpp"

#include "ScriptMenuUtils.h"
#include "ReplayScriptUtils.hpp"

#include "Util/UI.hpp"
#include "Util/Math.hpp"
#include "Util/String.hpp"
#include "Util/Misc.hpp"
#include "Util/Game.hpp"

#include <menukeyboard.h>
#include <format>

namespace GhostReplay {
    struct SFormattedTrackData {
        size_t NumCompatibleReplays;
        std::vector<std::string> Content;
    };

    const std::vector<std::string>& FormatTrackData(NativeMenu::Menu& mbCtx, CReplayScript& context, const CTrackData& track);
    bool EvaluateInput(std::string& searchFor);
    void UpdateTrackFilter(CReplayScript& context);
    void UpdateReplayFilter(CReplayScript& context);
    void CreateReplayVehicle(CReplayData* replay);

    bool trackSearchSelected = false;
    std::string trackSelectSearch;

    std::vector<CTrackData> filteredTracks;
    std::vector<CTrackData> filteredARSTracks;

    bool replaySearchSelected = false;
    std::string replaySelectSearch;

    std::vector<std::shared_ptr<CReplayData>> filteredReplays;
    std::unordered_map<std::string, SFormattedTrackData> formattedTrackData;

    const std::vector<std::string> lightsOptions = { "Default", "Force Off", "Force On" };
    const std::vector<std::string> roofOptions = { "Default", "Force Down", "Force Up" };
    const std::vector<std::string> syncOptions = { "Absolute", "Approximate" };
    const std::vector<std::vector<std::string>> syncDescriptions = {
        { "Absolute: Perfect playback, but lacks effects like tyre trails, dirt and smoke. Good for hotlaps." },
        { "Approximate: Effects are present, but vehicle may get knocked off-course by terrain or props." },
    };
    const std::vector<std::string> replaySortOptions = { "Name", "Lap time", "Date" };
    const std::vector<std::vector<std::string>> replaySortDescriptions = {
        { "Sorts by replay name alphabetically, ascending." },
        { "Sorts by lap time, ascending (fastest first)." },
        { "Sorts by date, ascending (oldest first)." }
    };
    const std::vector<std::string> fileModeOptions = { "JSON (Pretty)", "JSON (Minified)", "Binary" };
    const std::vector<std::vector<std::string>> fileModeDescriptions = {
        { "Saved as text, neatly separated with newlines. Good for editing manually. Uses a massive amount of storage." },
        { "Saved as text, no newlines. Manual edit possible. Uses a large amount of storage." },
        { "Saved in binary. Manual edits not trivial. Uses minimal storage." }
    };

}

std::vector<CScriptMenu<CReplayScript>::CSubmenu> GhostReplay::BuildMenu() {
    std::vector<CScriptMenu<CReplayScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Ghost Replay");
        mbCtx.Subtitle(std::format("~b~{}{}", Constants::DisplayVersion, GIT_DIFF));

        if (GhostReplay::ReplaysLocked()) {
            mbCtx.Option(std::format("Loading replays ({}/{})",
                GhostReplay::ReplaysLoaded(), GhostReplay::ReplaysTotal()), {
                    "Please wait, replays are loading.",
                    "Currently processing:",
                    GhostReplay::CurrentLoadingReplay()
                });
            return;
        }

        auto activeReplays = context.ActiveReplays();
        CTrackData* activeTrack = context.ActiveTrack();
        auto scriptMode = context.ScriptMode();

        std::string currentTrackName = "None";
        if (activeTrack)
            currentTrackName = activeTrack->Name;

        std::string currentGhostName = "None";
        std::vector<std::string> ghostDetails;
        if (activeReplays.size() == 1) {
            auto activeReplay = activeReplays[0];
            currentGhostName = activeReplay->Name;
            ghostDetails = {
                std::format("Track: {}", activeReplay->Track),
                std::format("Vehicle: {} {}",
                    Util::GetVehicleMake(activeReplay->VehicleModel),
                    Util::GetVehicleName(activeReplay->VehicleModel)),
                std::format("Time: {}", Util::FormatMillisTime(activeReplay->Nodes.back().Timestamp))
            };
        }
        else if (activeReplays.size() > 1) {
            currentGhostName = std::format("{} ghosts", activeReplays.size());
            ghostDetails.push_back(std::format("Track: {}", activeReplays[0]->Track));
            ghostDetails.push_back(std::format("{} replays:", activeReplays.size()));
            for (const auto& replay : activeReplays) {
                if (ghostDetails.size() > 5) {
                    ghostDetails.push_back("...");
                    break;
                }
                ghostDetails.push_back(std::format(" {} {} ({})",
                    Util::GetVehicleMake(replay->VehicleModel),
                    Util::GetVehicleName(replay->VehicleModel),
                    Util::FormatMillisTime(replay->Nodes.back().Timestamp)));
            }
        }

        if (mbCtx.MenuOption(std::format("Track: {}", currentTrackName), "trackselectmenu")) {
            UpdateTrackFilter(context);
        }

        if (mbCtx.MenuOption(std::format("Ghost: {}", currentGhostName), "ghostselectmenu", ghostDetails)) {
            UpdateReplayFilter(context);
        }

        mbCtx.MenuOption("Ghost controls", "ghostoptionsmenu");

        bool replaying = context.GetReplayState() != EReplayState::Idle;
        std::string replayAbortOption = replaying ? "Cancel playing ghosts" : "~m~Cancel playing ghosts";
        std::vector<std::string> replayAbortDetail;
        if (replaying) {
            replayAbortDetail = { "Currently replaying a ghost.",
                "Want to re-do your lap, but not cross the finish line? Use this to stop and reset the ghost." };
        }
        if (mbCtx.Option(replayAbortOption, replayAbortDetail)) {
            if (replaying)
                context.StopAllReplays();
        }

        bool recording = context.IsRecording();
        std::string recordAbortOption = recording ? "Cancel recording" : "~m~Cancel recording";
        std::vector<std::string> recordAbortDetail;
        if (recording) {
            recordAbortDetail = { "Currently recording a ghost replay.",
                "Not racing anymore? Use this to stop and discard the recording." };
        }
        if (mbCtx.Option(recordAbortOption, recordAbortDetail)) {
            if (recording)
                context.StopRecording();
        }

        if (mbCtx.MenuOption("Track setup", "tracksetupmenu")) {
            context.SetScriptMode(EScriptMode::DefineTrack);
        }
        else {
            context.SetScriptMode(EScriptMode::ReplayActive);
        }

        if (!context.GetUnsavedRuns().empty()) {
            mbCtx.MenuOption("Unsaved recordings", "unsavedrunsmenu",
                { "Manage recordings that aren't saved to disk.",
                  std::format("{} unsaved recording{}.",
                      context.GetUnsavedRuns().size(),
                      context.GetUnsavedRuns().size() != 1 ? "s" : "")});
        }
        else {
            mbCtx.Option("~m~Unsaved recordings",
                { "Manage recordings that aren't saved to disk.",
                  "No unsaved recordings to view or save." });
        }
        mbCtx.MenuOption("Settings", "settingsmenu");
    });

    /* mainmenu -> tracksetupmenu */
    submenus.emplace_back("tracksetupmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Track setup");
        mbCtx.Subtitle("");

        CTrackData* activeTrack = context.ActiveTrack();
        std::string currentTrackName = "None";
        if (activeTrack)
            currentTrackName = activeTrack->Name;

        if (activeTrack && activeTrack->FileName() == "ARS") {
            mbCtx.Option(std::format("~r~ARS track: {}", currentTrackName),
                { "Tracks from ARS cannot be edited." });
            return;
        }

        std::vector<std::string> trackNameOptDescr;
        if (activeTrack) {
            trackNameOptDescr = {
                "Enter a name to create a new track."
            };
        }
        else {
            trackNameOptDescr = {
                "Rename the track.",
                "Press enter to edit."
            };
        }

        if (mbCtx.Option(std::format("Track name: {}", currentTrackName), trackNameOptDescr)) {
            std::string newName = currentTrackName == "None" ? "" : currentTrackName;
            if (MenuUtils::GetKbString(newName)) {
                if (activeTrack) {
                    activeTrack->Name = newName;
                    UI::Notify(std::format("Renamed track to {}", newName), false);
                    GhostReplay::LoadTracks();
                    context.SetTrack(newName);
                }
                else {
                    CTrackData newTrack("");
                    newTrack.Name = newName;
                    newTrack.Write();
                    GhostReplay::LoadTracks();
                    context.SetTrack(newName);
                    UI::Notify(std::format("Created new track: {}", newName), false);
                }
            }
        }

        if (activeTrack) {
            Vector3 emptyVec{};
            if (activeTrack->StartLine.A != emptyVec &&
                activeTrack->StartLine.B != emptyVec) {
                UI::DrawSphere(activeTrack->StartLine.A, 0.25f, 255, 255, 255, 255);
                UI::DrawSphere(activeTrack->StartLine.B, 0.25f, 255, 255, 255, 255);
                UI::DrawLine(activeTrack->StartLine.A, activeTrack->StartLine.B, 255, 255, 255, 255);
            }

            if (activeTrack->FinishLine.A != emptyVec &&
                activeTrack->FinishLine.B != emptyVec) {
                UI::DrawSphere(activeTrack->FinishLine.A, 0.25f, 255, 255, 255, 255);
                UI::DrawSphere(activeTrack->FinishLine.B, 0.25f, 255, 255, 255, 255);
                UI::DrawLine(activeTrack->FinishLine.A, activeTrack->FinishLine.B, 255, 255, 255, 255);
            }

            const std::vector<std::string> startLineDescr = {
                "Set or move the start line."
            };
            const std::vector<std::string> finishLineDescr = {
                "Set or move the finish line."
            };
            const std::vector<std::string> saveDescr = {
                "Save and activate the track."
            };

            if (mbCtx.Option("Set start line", startLineDescr)) {
                SLineDef newLineDef{};
                if (context.StartLineDef(newLineDef, "start")) {
                    activeTrack->StartLine = newLineDef;
                }
            }

            if (mbCtx.Option("Set finish line", finishLineDescr)) {
                SLineDef newLineDef{};
                if (context.StartLineDef(newLineDef, "finish")) {
                    activeTrack->FinishLine = newLineDef;
                }
            }

            if (mbCtx.Option("Save track", saveDescr)) {
                std::string missingPoints;

                if (activeTrack->StartLine.A == emptyVec)
                    missingPoints = std::format("{}, {}", missingPoints, "Start left");
                if (activeTrack->StartLine.B == emptyVec)
                    missingPoints = std::format("{}, {}", missingPoints, "Start right");
                if (activeTrack->FinishLine.A == emptyVec)
                    missingPoints = std::format("{}, {}", missingPoints, "Finish left");
                if (activeTrack->FinishLine.B == emptyVec)
                    missingPoints = std::format("{}, {}", missingPoints, "Finish right");

                if (!missingPoints.empty()) {
                    UI::Notify(std::format("Couldn't save track. Missing points: {}", missingPoints), true);
                    return;
                }

                activeTrack->Write();
                UI::Notify(std::format("Saved track {}", activeTrack->Name), true);
                GhostReplay::LoadTracks();
                context.SetTrack(activeTrack->Name);
            }
        }
    });

    /* mainmenu -> trackselectmenu */
    submenus.emplace_back("trackselectmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Tracks");
        mbCtx.Subtitle("");

        if (context.GetTracks().empty() && context.GetARSTracks().empty()) {
            mbCtx.Option("No tracks");
            return;
        }

        if (trackSearchSelected) {
            if (EvaluateInput(trackSelectSearch)) {
                UpdateTrackFilter(context);
            }
        }

        std::vector<std::string> searchExtra {
            "Type to search. Use Delete for backspace.",
            "Searching for:",
            trackSelectSearch
        };
        if (mbCtx.OptionPlus("Search...", searchExtra, &trackSearchSelected, nullptr, nullptr, "Search",
            { "Matches track name and description." })) {
            UpdateTrackFilter(context);
        }

        for (auto& track : filteredTracks) {
            bool currentTrack = false;
            if (context.ActiveTrack())
                currentTrack = context.ActiveTrack()->Name == track.Name;
            std::string selector = currentTrack ? "-> " : "";
            std::string deleteCol = track.MarkedForDeletion ? "~r~" : "";

            std::vector<std::string> description{
                "Press select to toggle track selection.",
            };

            if (!track.MarkedForDeletion) {
                description.emplace_back("Press Right to teleport to starting point.");
                description.emplace_back("Press Left to delete.");
            }
            else {
                description.emplace_back("Press Right to unmark deletion.");
                description.emplace_back("~r~Press Left to confirm deletion.");
                description.emplace_back("~r~~h~THIS IS PERMANENT");
            }

            auto onRight = [&]() {
                if (!track.MarkedForDeletion) {
                    context.TeleportToTrack(track);
                }

                track.MarkedForDeletion = false;
            };
            auto deleteFlag = [&]() {
                if (track.MarkedForDeletion) {
                    context.DeleteTrack(track);
                    UpdateTrackFilter(context);
                }
                else {
                    track.MarkedForDeletion = true;
                }
            };

            std::string optionName = std::format("{}{}{}", deleteCol, selector, track.Name);
            bool highlighted;
            if (mbCtx.OptionPlus(optionName, {}, &highlighted, onRight, deleteFlag, "", description)) {
                if (!currentTrack)
                    context.SetTrack(track.Name);
                else
                    context.SetTrack("");
            }

            if (highlighted) {
                auto& extras = FormatTrackData(mbCtx, context, track);
                mbCtx.OptionPlusPlus(extras, track.Name);
            }
        }

        for (auto& track : filteredARSTracks) {
            bool currentTrack = false;
            if (context.ActiveTrack())
                currentTrack = context.ActiveTrack()->Name == track.Name;
            std::string selector = currentTrack ? "-> " : "";

            std::vector<std::string> description{
                "Press select to toggle track selection.",
                "Press Right to teleport to starting point.",
            };

            auto onRight = [&]() {
                context.TeleportToTrack(track);
            };

            std::string optionName = std::format("{}{}", selector, track.Name);
            bool highlighted;
            if (mbCtx.OptionPlus(optionName, {}, &highlighted, onRight, nullptr, "", description)) {
                if (!currentTrack)
                    context.SetTrack(track.Name);
                else
                    context.SetTrack("");
            }

            if (highlighted) {
                auto& extras = FormatTrackData(mbCtx, context, track);
                mbCtx.OptionPlusPlus(extras, track.Name);
            }
        }

    });

    /* mainmenu -> ghostselectmenu */
    submenus.emplace_back("ghostselectmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Ghosts");

        if (!context.ActiveTrack()) {
            mbCtx.Subtitle("");
            mbCtx.Option("No track selected", { "A track needs to be selected before a ghost can be chosen." });
            return;
        }

        mbCtx.Subtitle(context.ActiveTrack()->Name);

        if (context.GetReplays().empty()) {
            mbCtx.Option("No ghosts");
            return;
        }

        if (mbCtx.StringArray("Sort by", replaySortOptions, GetSettings().Main.ReplaySortBy,
            replaySortDescriptions[GetSettings().Main.ReplaySortBy])) {
            UpdateReplayFilter(context);
        }

        if (replaySearchSelected) {
            if (EvaluateInput(replaySelectSearch)) {
                UpdateReplayFilter(context);
            }
        }

        std::vector<std::string> searchExtra{
            "Type to search. Use Delete for backspace.",
            "Searching for:",
            replaySelectSearch
        };
        if (mbCtx.OptionPlus("Search...", searchExtra, &replaySearchSelected, nullptr, nullptr, "Search",
            { "Matches replay name, vehicle name, track time and recording time." })) {
            UpdateReplayFilter(context);
        }

        for (auto& replay : filteredReplays) {
            bool currentReplay = false;
            const auto& activeReplays = context.ActiveReplays();
            currentReplay = std::find_if(activeReplays.begin(), activeReplays.end(),
                [&](const CReplayData* activeReplay) {
                    return activeReplay->Name == replay->Name &&
                        activeReplay->Timestamp == replay->Timestamp;
                }
            ) != activeReplays.end();

            std::string selector = currentReplay ? "-> " : "";
            std::string deleteCol = replay->MarkedForDeletion ? "~r~" : "";

            std::vector<std::string> description{
                "Press select to toggle replay selection.",
            };

            if (!replay->MarkedForDeletion) {
                description.emplace_back("Press Left to delete.");
                description.emplace_back("Press Right to spawn a copy of the replay vehicle.");
            }
            else {
                description.emplace_back("Press Right to unmark deletion.");
                description.emplace_back("~r~Press Left to confirm deletion.");
                description.emplace_back("~r~~h~THIS IS PERMANENT");
            }

            auto clearDeleteFlag = [&]() {
                if (replay->MarkedForDeletion == false) {
                    CreateReplayVehicle(replay.get());
                }
                else {
                    replay->MarkedForDeletion = false;
                }
            };

            bool triggerBreak = false;
            auto deleteFlag = [&]() {
                if (replay->MarkedForDeletion) {
                    context.DeleteReplay(*replay);
                    triggerBreak = true;
                    UpdateReplayFilter(context);
                }
                else {
                    replay->MarkedForDeletion = true;
                }
            };

            std::string optionNameRaw = std::format("{} - {}",
                Util::FormatMillisTime(replay->Nodes.back().Timestamp),
                Util::GetVehicleName(replay->VehicleModel));
            std::string optionName = std::format("{}{}{}", deleteCol, selector, optionNameRaw);
            std::string datetime;
            if (replay->Timestamp == 0) {
                datetime = "No date";
            }
            else {
                datetime = Util::GetTimestampReadable(replay->Timestamp);
            }
            std::vector<std::string> extras
            {
                std::format("Track: {}", replay->Track),
                std::format("Vehicle: {} {}",
                    Util::GetVehicleMake(replay->VehicleModel),
                    Util::GetVehicleName(replay->VehicleModel)),
                std::format("Time: {}",  Util::FormatMillisTime(replay->Nodes.back().Timestamp)),
                std::format("Lap recorded: {}", datetime),
            };

            if (mbCtx.OptionPlus(optionName, extras, nullptr, clearDeleteFlag, deleteFlag, "Ghost", description)) {
                if (!currentReplay)
                    context.SelectReplay(replay->Name, replay->Timestamp);
                else
                    context.DeselectReplay(replay->Name, replay->Timestamp);
            }
            if (triggerBreak)
                break;
        }

        if (mbCtx.Option("Deselect all replays")) {
            context.StopAllReplays();
            context.ClearSelectedReplays();
        }
    });

    /* mainmenu -> unsavedrunsmenu */
    submenus.emplace_back("unsavedrunsmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Unsaved runs");
        mbCtx.Subtitle("");

        if (context.GetUnsavedRuns().empty()) {
            mbCtx.Option("No unsaved runs");
            return;
        }

        auto unsavedRunIt = context.GetUnsavedRuns().begin();
        while (unsavedRunIt != context.GetUnsavedRuns().end()) {
            std::string runName = Util::FormatReplayName(
                unsavedRunIt->Nodes.back().Timestamp,
                unsavedRunIt->Track,
                Util::GetVehicleName(unsavedRunIt->VehicleModel));

            std::string datetime;
            if (unsavedRunIt->Timestamp == 0) {
                datetime = "No date";
            }
            else {
                datetime = Util::GetTimestampReadable(unsavedRunIt->Timestamp);
            }

            std::vector<std::string> ghostDetails = {
                "Select to save ghost.",
                std::format("Track: {}", unsavedRunIt->Track),
                std::format("Vehicle: {} {}",
                    Util::GetVehicleMake(unsavedRunIt->VehicleModel),
                    Util::GetVehicleName(unsavedRunIt->VehicleModel)),
                std::format("Time: {}", Util::FormatMillisTime(unsavedRunIt->Nodes.back().Timestamp)),
                std::format("Lap recorded: {}", datetime),
            };

            if (mbCtx.Option(std::format("{}", runName), ghostDetails)) {
                CReplayData& unsavedRun = *unsavedRunIt;
                unsavedRun.Name = runName;

                CReplayData::WriteAsync(unsavedRun);
                GhostReplay::AddReplay(unsavedRun);
                context.AddCompatibleReplay(unsavedRun);

                unsavedRunIt = context.EraseUnsavedRun(unsavedRunIt);
            }
            else {
                ++unsavedRunIt;
            }
        }

        if (mbCtx.Option("Clear unsaved runs")) {
            context.ClearUnsavedRuns();
        }
    });

    /* mainmenu -> ghostoptionsmenu */
    submenus.emplace_back("ghostoptionsmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Ghost controls");
        mbCtx.Subtitle("");

        float offset = static_cast<float>(GetSettings().Replay.OffsetSeconds);
        bool offsetChanged = mbCtx.FloatOptionCb("Ghost offset", offset, -60.0f, 60.0f, 0.1f,
            MenuUtils::GetKbFloat,
            { "Delay of the ghost car. Positive values put it ahead, negative values delay.",
              "In seconds." });
        if (offsetChanged) {
            GetSettings().Replay.OffsetSeconds = static_cast<double>(offset);
            auto newTime = context.GetReplayProgress() + offset * 1000.0;
            for (auto& replayVehicle : context.GetReplayVehicles()) {
                if (replayVehicle->GetReplayState() != EReplayState::Idle)
                    replayVehicle->SetReplayTime(newTime);
            }

        }

        const std::vector<std::string> replayAlphaDescr{
            "The transparency of the ghost vehicle.",
            "0%: completely invisible.",
            "100%: completely visible.",
        };
        int replayAlpha = GetSettings().Replay.VehicleAlpha;
        if (mbCtx.IntOptionCb("Ghost opacity %", replayAlpha, 0, 100, 20, MenuUtils::GetKbInt, replayAlphaDescr)) {
            if (replayAlpha < 0)
                replayAlpha = 0;
            if (replayAlpha > 100)
                replayAlpha = 100;
            GetSettings().Replay.VehicleAlpha = replayAlpha;
            for (auto& replayVehicle : context.GetReplayVehicles()) {
                ENTITY::SET_ENTITY_ALPHA(
                    replayVehicle->GetVehicle(),
                    map(GetSettings().Replay.VehicleAlpha, 0, 100, 0, 255), false);
                if (GetSettings().Replay.VehicleAlpha == 100) {
                    ENTITY::RESET_ENTITY_ALPHA(replayVehicle->GetVehicle());
                }
            }
        }

        mbCtx.StringArray("Lights", lightsOptions, GetSettings().Replay.ForceLights,
            { "Default: Replay lights as recorded.",
              "Forced Off: Always lights off.",
              "Forced On: Always use low beam.", });

        if (context.ActiveReplays().empty()) {
            mbCtx.Option("~m~Replay controls unavailable", { "Select a track and a replay." });
        }
        else {
            const double scrubDist = GetSettings().Replay.ScrubDistanceSeconds * 1000.0; // milliseconds
            const auto& activeReplays = context.ActiveReplays();
            const auto& replayVehicles = context.GetReplayVehicles();
            EReplayState replayState = context.GetReplayState();

            std::string replayStateName;
            switch (replayState) {
                case EReplayState::Paused:  replayStateName = "Paused"; break;
                case EReplayState::Playing: replayStateName = "Playing"; break;
                case EReplayState::Idle:    // Default
                default:                    replayStateName = "Stopped";
            }

            std::vector<std::string> playbackDetails;

            if (activeReplays.size() == 1 && replayVehicles.size() == 1) {
                auto activeReplay = activeReplays[0];
                playbackDetails = {
                    std::format("{} on {}", Util::GetVehicleName(activeReplay->VehicleModel), activeReplay->Track),
                    replayStateName,
                    std::format("{} / {}",
                        Util::FormatMillisTime(replayVehicles[0]->GetReplayProgress()),
                        Util::FormatMillisTime(activeReplay->Nodes.back().Timestamp)),
                };
            }
            else if (activeReplays.size() > 1) {
                auto numGhosts = replayVehicles.size();
                auto numStillDriving = 0;
                for(const auto& replayVehicle : replayVehicles) {
                    numStillDriving += replayVehicle->GetReplayState() == EReplayState::Idle ? 0 : 1;
                }
                
                playbackDetails = {
                    std::format("{} ghosts on {}", activeReplays.size(), context.ActiveTrack()->Name),
                    std::format("{} - {} / {}",
                        replayStateName,
                        Util::FormatMillisTime(context.GetReplayProgress()),
                        Util::FormatMillisTime(context.GetSlowestActiveReplay())),
                    std::format("{} out of {} driving", numStillDriving, numGhosts),
                };

                for (const auto& replayVehicle : replayVehicles) {
                    playbackDetails.push_back(
                        std::format("{} - {} / {}",
                            Util::GetVehicleName(replayVehicle->GetReplay()->VehicleModel),
                            Util::FormatMillisTime(replayVehicle->GetReplayProgress()),
                            Util::FormatMillisTime(replayVehicle->GetReplay()->Nodes.back().Timestamp))
                    );
                }
            }

            std::string secondsFormat = std::format("{:.3f} second{}",
                GetSettings().Replay.ScrubDistanceSeconds,
                Math::Near(GetSettings().Replay.ScrubDistanceSeconds, 1.0, 0.0005) ? "" : "s");
            std::string playbackDescription =
                std::format("Playback controls. Select to play or pause. "
                    "Press left and right to jump backward or forward {}.", secondsFormat);

            bool togglePause = mbCtx.OptionPlus(
                std::format("<< {} >>", replayState != EReplayState::Playing ? "Play" : "Pause"),
                playbackDetails,
                nullptr,
                [&]() { context.ScrubForward(scrubDist); },
                [&]() { context.ScrubBackward(scrubDist); },
                "Playback controls",
                { playbackDescription });
            if (togglePause) {
                context.TogglePause(replayState == EReplayState::Playing);
            }

            if (GetSettings().Main.Debug && activeReplays.size() == 1 && replayVehicles.size() == 1) {
                bool togglePause = mbCtx.OptionPlus(
                    std::format("<< [{}/{}] >>", context.GetFrameIndex(), context.GetNumFrames() - 1),
                    playbackDetails,
                    nullptr,
                    [&]() { context.FrameNext(); },
                    [&]() { context.FramePrev(); },
                    "Frame controls",
                    { "Frame controls. Select to play/pause. Left = previous frame. Right = next frame." });
                if (togglePause) {
                    context.TogglePause(replayState == EReplayState::Playing);
                }
            }
            else if (GetSettings().Main.Debug) {
                bool togglePause = mbCtx.OptionPlus(
                    std::format("<< {} >>", replayState != EReplayState::Playing ? "Play" : "Pause"),
                    playbackDetails,
                    nullptr,
                    [&]() { context.ScrubForward(5.0); },
                    [&]() { context.ScrubBackward(5.0); },
                    "Step controls",
                    { "Frame controls replaced by 5ms step for multiple vehicles." });
                if (togglePause) {
                    context.TogglePause(replayState == EReplayState::Playing);
                }
            }

            bool replaying = replayState != EReplayState::Idle;
            std::string replayAbortOption = replaying ? "Stop playback" : "~m~Stop playback";
            if (mbCtx.Option(replayAbortOption)) {
                if (replaying)
                    context.StopAllReplays();
            }

            bool passengerModeAvailable = context.ActiveTrack() && context.ActiveReplays().size() > 0;

            if (passengerModeAvailable) {
                bool active = context.IsPassengerModeActive();
                std::string optionText = active ?
                    "<< Stop spectating >>" :
                    "<< Spectate >>";

                std::vector<std::string> passengerDetails;
                for (const auto& replayVehicle : context.GetReplayVehicles()) {
                    std::string selectMarker;
                    if (replayVehicle.get() == context.GetPassengerVehicle()) {
                        selectMarker = "-> ";
                    }
                    passengerDetails.push_back(std::format("{}{} - {} ({})",
                        selectMarker,
                        Util::GetVehicleName(replayVehicle->GetReplay()->VehicleModel),
                        Util::FormatMillisTime(replayVehicle->GetReplay()->Nodes.back().Timestamp),
                        replayVehicle->GetReplayState() == EReplayState::Idle ? "Stopped" : "Playing"));
                }

                bool toggle = mbCtx.OptionPlus(
                    optionText,
                    passengerDetails,
                    nullptr,
                    [&]() { context.PassengerVehicleNext(); },
                    [&]() { context.PassengerVehiclePrev(); },
                    "Spectating",
                    { "Select to enter or exit spectator mode.",
                      "Press left and right to select different vehicles.",
                      "Spectator mode places you in the passenger seat of the replay vehicle." });
                if (toggle) {
                    if (active)
                        context.DeactivatePassengerMode();
                    else
                        context.ActivatePassengerMode();
                }
            }
            else {
                mbCtx.Option("~m~Spectator mode unavailable",
                    { "Spectator mode is available when a track and one or more replays are selected." });
            }
        }
    });

    /* mainmenu -> settingsmenu */
    submenus.emplace_back("settingsmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Settings");
        mbCtx.Subtitle("");

        mbCtx.BoolOption("Notify laptimes", GetSettings().Main.NotifyLaps,
            { "Show a notification when finishing a lap.",
              "Yellow: Normal",
              "Green: Faster than the ghost lap.",
              "Purple: All-time fastest lap." });

        mbCtx.BoolOption("Draw start/finish", GetSettings().Main.DrawStartFinish,
            { "Draws green flares for the track start, red flares for the track finish.",
              "Setting applies next time a track is selected." });

        mbCtx.BoolOption("Ghost blips", GetSettings().Main.GhostBlips,
            { "Draws blips for the ghost vehicle.",
              "Setting applies next time a ghost is started." });

        mbCtx.BoolOption("Track blips", GetSettings().Main.StartStopBlips,
            { "Draws blips for the start and finish.",
              "Setting applies next time a track is selected.",
              "Draws separate start (blue) and finish (white) blips when they're further apart than 5 meters." });

        mbCtx.BoolOption("Show recording time", GetSettings().Main.ShowRecordTime,
            { "Shows the time since the lap/recording started.",
              "Appears on the top left of the screen, when a map is selected." });

        //mbCtx.BoolOption("Broadcast to RagePresence", GetSettings().Main.RagePresence,
        //    { "Display the current race stats on Discord using RagePresence by Lemon." });

        mbCtx.MenuOption("Recording settings", "recordsettingsmenu");
        mbCtx.MenuOption("Advanced ghost settings", "advghostsettingsmenu");

        if (mbCtx.Option("Refresh tracks and ghosts",
            { "Refresh tracks and ghosts if they are changed outside the script." })) {
            context.StopRecording();
            context.StopAllReplays();
            context.ClearSelectedReplays();

            GhostReplay::LoadTracks();
            GhostReplay::LoadARSTracks();
            GhostReplay::LoadReplays();
            GhostReplay::LoadTrackImages();
            formattedTrackData.clear();
            mbCtx.CloseMenu();
        }

        mbCtx.BoolOption("Developer mode", GetSettings().Main.Debug,
            { "Shows dispay elements and options used during development." });
        });

    /* mainmenu -> settingsmenu -> recordsettingsmenu */
    submenus.emplace_back("recordsettingsmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Recording settings");
        mbCtx.Subtitle("");

        const std::string AutoGhostFasterOption = "Automatically save lap (faster)";
        const std::string AutoGhostAllOption = "Automatically save lap (all)";

        const std::vector<std::string> AutoGhostFasterExtra =
        { "Laps faster than the current playing ghost are saved automatically.",
          std::format("Not compatible with \"{}\"", AutoGhostAllOption) };

        const std::vector<std::string> AutoGhostAllExtra =
        { "Laps are saved automatically.",
          "Select two or more replays to stack new recordings on top.",
          std::format("Not compatible with \"{}\"", AutoGhostFasterOption) };

        if (!GetSettings().Record.AutoGhostAll) {
            mbCtx.BoolOption(AutoGhostFasterOption, GetSettings().Record.AutoGhostFast,
                AutoGhostFasterExtra);
        }
        else {
            mbCtx.Option(std::format("~m~{}", AutoGhostFasterOption),
                AutoGhostFasterExtra);
        }

        if (!GetSettings().Record.AutoGhostFast) {
            mbCtx.BoolOption(AutoGhostAllOption, GetSettings().Record.AutoGhostAll,
                AutoGhostAllExtra);
        }
        else {
            mbCtx.Option(std::format("~m~{}", AutoGhostAllOption),
                AutoGhostAllExtra);
        }

        std::vector<std::string> timestepDescr{
            "The minimum time in milliseconds between each ghost recording sample.",
            "The lower, the more accurate the ghost playback will be, but might increase file size "
                "and impact performance.",
        };
        int deltaMillis = GetSettings().Record.DeltaMillis;
        if (mbCtx.IntOptionCb("Recording timestep", deltaMillis, 0, 1000, 1, MenuUtils::GetKbInt, timestepDescr)) {
            GetSettings().Record.DeltaMillis = deltaMillis;
        }

        mbCtx.BoolOption("Record lights", GetSettings().Record.Optional.Lights,
            { "Records lights (low beam and high beam) when enabled." });

        mbCtx.BoolOption("Record indicators", GetSettings().Record.Optional.Indicators,
            { "Records indicators (left, right and hazard) when enabled." });

        mbCtx.BoolOption("Record emergency lights/siren", GetSettings().Record.Optional.Siren,
            { "Records emergency lights/siren when enabled. Only records on/off, doesn't record fast/slow.",
              "Keep in mind the siren is only audible if the playback vehicle has a driver." });

        mbCtx.StringArray("File mode", fileModeOptions, GetSettings().Record.StorageType,
                          fileModeDescriptions[GetSettings().Record.StorageType]);
    });

    /* mainmenu -> settingsmenu -> advghostsettingsmenu */
    submenus.emplace_back("advghostsettingsmenu", [](NativeMenu::Menu& mbCtx, CReplayScript& context) {
        mbCtx.Title("Ghost settings");
        mbCtx.Subtitle("Advanced");

        mbCtx.BoolOption("Auto-load quickest ghost", GetSettings().Replay.AutoLoadGhost, 
            { "Automatically loads the quickest ghost lap when a track is selected, for that specific vehicle model." });

        float scrubDist = static_cast<float>(GetSettings().Replay.ScrubDistanceSeconds);
        bool scrubDistChanged = mbCtx.FloatOptionCb("Scrub distance", scrubDist, 0.010f, 60.0f, 0.010f,
            MenuUtils::GetKbFloat,
            { "How far to skip back and forth, in seconds.",
              "Hint: Press enter/select to manually input value." });
        if (scrubDistChanged) {
            GetSettings().Replay.ScrubDistanceSeconds = static_cast<double>(scrubDist);
        }

        std::string fallbackOpt =
            std::format("Fallback vehicle model: {}", GetSettings().Replay.FallbackModel);
        std::vector<std::string> fallbackDescr{
            "When the vehicle model from a replay file is missing, replace it with this model."
        };
        if (mbCtx.Option(fallbackOpt, fallbackDescr)) {
            std::string value = GetSettings().Replay.FallbackModel;
            if (MenuUtils::GetKbString(value)) {
                GetSettings().Replay.FallbackModel = value;
            }
        }

        mbCtx.BoolOption("Force fallback model", GetSettings().Replay.ForceFallbackModel, 
            { "Always use the fallback model." });

        mbCtx.BoolOption("Zero velocity on pause", GetSettings().Replay.ZeroVelocityOnPause,
            { "When paused, no velocity is set. This stops the third person gameplay camera from moving, "
              "but will cause glitched suspension for vehicles with advanced handling flag 0x0800000." });

        auto syncType = as_int(GetSettings().Replay.SyncType);
        const auto& description = syncDescriptions[syncType];
        if (mbCtx.StringArray("Replay sync type", syncOptions, syncType, description)) {
            GetSettings().Replay.SyncType = static_cast<ESyncType>(syncType);
            for (auto& replayVehicle : context.GetReplayVehicles()) {
                if (replayVehicle->GetReplayState() != EReplayState::Idle) {
                    replayVehicle->UpdateCollision(GetSettings().Replay.SyncType != ESyncType::Constant);
                }
            }
        }

        if (GetSettings().Replay.SyncType == ESyncType::Distance) {
            mbCtx.FloatOptionCb("Resync distance", GetSettings().Replay.SyncDistance, 0.01f, 1.00f, 0.01f,
                MenuUtils::GetKbFloat,
                { "When the vehicle drifts too far off its intended position, it needs to re-sync positions.",
                  "This value is the drift distance after which it re-syncs, in meters." });

            mbCtx.FloatOptionCb("Sync compensation", GetSettings().Replay.SyncCompensation, 0.00f, 50.00f, 0.10f,
                MenuUtils::GetKbFloat,
                { "Compensate for drift by pulling the vehicle to its intended position.",
                  "This value is the pull strength." });
        }

        mbCtx.StringArray("Force roof state", roofOptions, GetSettings().Replay.ForceRoof,
            { "Default: Replay roof as recorded.",
              "Forced Down: Always put the roof down.",
              "Forced Up: Always put the roof up.", });

        bool toggleDrivers = mbCtx.BoolOption("Enable drivers", GetSettings().Replay.EnableDrivers,
            { "Spawns drivers in the replay vehicles.",
              "While this allows things like active spoilers to work, "
              "it does severely reduce engine audio quality, so it is disabled by default." });

        if (toggleDrivers) {
            context.ToggleDrivers(GetSettings().Replay.EnableDrivers);
        }

        if (GetSettings().Replay.EnableDrivers) {
            std::vector<std::string> extra{
                "Fallback models:",
            };

            if (GetSettings().Replay.DriverModels.empty()) {
                extra.push_back("None");
            }
            for (const auto& driverModel : GetSettings().Replay.DriverModels) {
                extra.push_back(driverModel);
            }

            extra.push_back("");
            extra.push_back("Change or add models in settings_general.ini");

            mbCtx.OptionPlus("Fallback models", extra, nullptr, nullptr, nullptr, "Models",
                { "Uses these fallback models for drivers, if the recording doesn't have a driver.",
                  "Model is randomly picked." });
        }

        std::vector<std::string> collisionDescription = { "Allow ghost cars to collide.",
              "Warning: Performance impact if ghost vehicles overlap!",
              "Use with caution." };

        if (!Impacts::Found()) {
            collisionDescription.insert(collisionDescription.begin(),
                { "~r~Unable to patch collisions, collisions currently always enabled!" });
        }

        mbCtx.BoolOption(std::format("{}Enable collision", Impacts::Found() ? "" : "~r~"),
            GetSettings().Replay.EnableCollision,
            collisionDescription);
    });

    return submenus;
}

const std::vector<std::string>& GhostReplay::FormatTrackData(NativeMenu::Menu& mbCtx, CReplayScript& context, const CTrackData& track) {
    auto compatibleReplays = context.GetCompatibleReplays(track.Name);

    auto formattedTrackDataIt = formattedTrackData.find(track.Name);
    if (formattedTrackDataIt != formattedTrackData.end()) {
        if (formattedTrackDataIt->second.NumCompatibleReplays == compatibleReplays.size()) {
            return formattedTrackDataIt->second.Content;
        }
        else {
            formattedTrackData.erase(formattedTrackDataIt);
        }
    }

    CReplayData fastestReplayInfo = context.GetFastestReplay(track.Name, 0);
    std::string lapRecordString = "No times set";
    if (!fastestReplayInfo.Name.empty()) {
        lapRecordString = std::format("{} ({})",
            Util::FormatMillisTime(fastestReplayInfo.Nodes.back().Timestamp),
            Util::GetVehicleName(fastestReplayInfo.VehicleModel));
    }

    auto imgStr = context.GetTrackImageMenuString(track.Name);
    if (!imgStr.empty()) {
        imgStr = std::format("{}{}", mbCtx.ImagePrefix, imgStr);
    }
    else {
        imgStr = "No preview";
    }

    std::vector<std::string> extras{
        imgStr,
        std::format("Description: {}", track.Description),
        std::format("Total ghosts: {}", compatibleReplays.size()),
        std::format("Lap record: {}", lapRecordString),
        "Fastest ghost of each model:",
    };

    // compatibleReplays is ordered quickest to slowest.
    std::sort(compatibleReplays.begin(), compatibleReplays.end(),
        [](const auto& a, const auto& b) {
            return a->Nodes.back().Timestamp < b->Nodes.back().Timestamp;
        });

    std::vector<Hash> fastestModels;
    for (const auto& replay : compatibleReplays) {
        if (std::find(fastestModels.begin(), fastestModels.end(), replay->VehicleModel) != fastestModels.end())
            continue;
        extras.push_back(std::format("{} ({})",
            Util::FormatMillisTime(replay->Nodes.back().Timestamp),
            Util::GetVehicleName(replay->VehicleModel)));
        fastestModels.push_back(replay->VehicleModel);
    }

    auto inserted = formattedTrackData.insert({ track.Name, { compatibleReplays.size(), extras } });
    return inserted.first->second.Content;
}

bool GhostReplay::EvaluateInput(std::string& searchFor) {
    using namespace NativeMenu;
    HUD::SET_PAUSE_MENU_ACTIVE(false);
    PAD::DISABLE_ALL_CONTROL_ACTIONS(2);

    for (char c = ' '; c < '~'; c++) {
        int key = NativeMenu::GetKeyFromName(std::string(1, c));
        if (key == -1) continue;
        if (IsKeyJustUp(key)) {
            searchFor += c;
            return true;
        }
    }

    if ((IsKeyDown(NativeMenu::GetKeyFromName("LSHIFT")) || IsKeyDown(NativeMenu::GetKeyFromName("RSHIFT"))) && IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_MINUS"))) {
        searchFor += '_';
        return true;
    }
    if (IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_MINUS"))) {
        searchFor += '-';
        return true;
    }
    if (IsKeyJustUp(NativeMenu::GetKeyFromName("SPACE"))) {
        searchFor += ' ';
        return true;
    }
    if (IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_COMMA"))) {
        searchFor += ',';
        return true;
    }
    if (IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_PERIOD"))) {
        searchFor += '.';
        return true;
    }
    if ((IsKeyDown(NativeMenu::GetKeyFromName("LSHIFT")) || IsKeyDown(NativeMenu::GetKeyFromName("RSHIFT"))) && IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_1"))) {
        searchFor += ':';
        return true;
    }
    if (IsKeyJustUp(NativeMenu::GetKeyFromName("VK_OEM_1"))) {
        searchFor += ';';
        return true;
    }

    if (IsKeyJustUp(NativeMenu::GetKeyFromName("DELETE")) && !searchFor.empty()) {
        searchFor.pop_back();
        return true;
    }

    return false;
}

void GhostReplay::UpdateTrackFilter(CReplayScript& context) {
    filteredTracks.clear();
    filteredARSTracks.clear();

    for (const auto& track : context.GetTracks()) {
        if (Util::FindSubstring(track.Name, trackSelectSearch) != -1 ||
            Util::FindSubstring(track.Description, trackSelectSearch) != -1) {
            filteredTracks.push_back(track);
        }
    }

    for (const auto& track : context.GetARSTracks()) {
        if (Util::FindSubstring(track.Name, trackSelectSearch) != -1 ||
            Util::FindSubstring(track.Description, trackSelectSearch) != -1) {
            filteredARSTracks.push_back(track);
        }
    }
}

void GhostReplay::UpdateReplayFilter(CReplayScript& context) {
    filteredReplays.clear();

    if (context.ActiveTrack() == nullptr)
        return;

    auto splitSearchTerms = Util::split(replaySelectSearch, ' ');

    for (const auto& replay : context.GetCompatibleReplays(context.ActiveTrack()->Name)) {
        std::string vehicleMake = Util::GetVehicleMake(replay->VehicleModel);
        std::string vehicleName = Util::GetVehicleName(replay->VehicleModel);
        std::string tracktime = Util::FormatMillisTime(replay->Nodes.back().Timestamp);
        std::string datetime;
        if (replay->Timestamp == 0) {
            datetime = "No date";
        }
        else {
            datetime = Util::GetTimestampReadable(replay->Timestamp);
        }

        uint32_t matchedTerms = 0;
        for (const auto& searchTerm : splitSearchTerms) {
            if (Util::FindSubstring(replay->Name, searchTerm) != -1 ||
                Util::FindSubstring(vehicleMake, searchTerm) != -1 ||
                Util::FindSubstring(vehicleName, searchTerm) != -1 ||
                Util::FindSubstring(tracktime, searchTerm) != -1 ||
                Util::FindSubstring(datetime, searchTerm) != -1) {
                matchedTerms++;
            }
        }

        if (matchedTerms == splitSearchTerms.size() || splitSearchTerms.empty()) {
            filteredReplays.push_back(replay);
        }
    }

    auto dateLess = [](const std::shared_ptr<CReplayData>& replaya,
        const std::shared_ptr<CReplayData>& replayb) {
            return replaya->Timestamp < replayb->Timestamp;
    };

    auto timeLess = [](const std::shared_ptr<CReplayData>& replaya,
        const std::shared_ptr<CReplayData>& replayb) {
        return replaya->Nodes.back().Timestamp < replayb->Nodes.back().Timestamp;
    };

    // Lap time
    if (GetSettings().Main.ReplaySortBy == 1) {
        std::sort(filteredReplays.begin(), filteredReplays.end(), timeLess);
    }
    // Date
    else if (GetSettings().Main.ReplaySortBy == 2) {
        std::sort(filteredReplays.begin(), filteredReplays.end(), dateLess);
    }
}

void GhostReplay::CreateReplayVehicle(CReplayData* replay) {
    Vector3 pos;
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    Vector3 dimMinReplay, dimMaxReplay;
    MISC::GET_MODEL_DIMENSIONS(replay->VehicleModel, &dimMinReplay, &dimMaxReplay);

    if (ENTITY::DOES_ENTITY_EXIST(playerVehicle)) {
        Vector3 dimMinPlr, dimMaxPlr;
        MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(playerVehicle), &dimMinPlr, &dimMaxPlr);
        float offsetRight = dimMaxPlr.x + 1.5f + -dimMinReplay.x;
        pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerVehicle, { offsetRight, 0.0f, 0.0f });
    }
    else {
        float offsetRight = 1.5f + -dimMinReplay.x;
        pos = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerPed, { offsetRight, 0.0f, 0.0f });
    }

    Vehicle vehicle = Util::CreateVehicle(replay->VehicleModel, &replay->VehicleMods, pos);
    if (vehicle == 0)
        return;

    ENTITY::SET_ENTITY_HEADING(vehicle, ENTITY::GET_ENTITY_HEADING(playerPed));
    ENTITY::SET_VEHICLE_AS_NO_LONGER_NEEDED(&vehicle);
}
