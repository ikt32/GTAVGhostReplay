#include "Game.hpp"
#include "Math.hpp"
#include "UI.hpp"
#include <format>

namespace Util {
    bool VehicleAvailable(Vehicle vehicle, Ped playerPed, bool checkSeat) {
        bool seatOk = true;
        if (checkSeat)
            seatOk = playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, 0);

        return vehicle != 0 &&
            ENTITY::DOES_ENTITY_EXIST(vehicle) &&
            PED::IS_PED_IN_VEHICLE(playerPed, vehicle, false) &&
            seatOk;
    }

    SBoxPoints GetBoxPoints(Vector3 pos, Vector3 rot, Vector3 fwd, Vector3 dimMax, Vector3 dimMin) {
        return {
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMin.x, 0,  dimMax.y, 0, dimMin.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMin.x, 0,  dimMax.y, 0, dimMax.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMax.x, 0,  dimMax.y, 0, dimMin.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMax.x, 0,  dimMax.y, 0, dimMax.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMin.x, 0, -dimMax.y, 0, dimMin.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMin.x, 0, -dimMax.y, 0, dimMax.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMax.x, 0, -dimMax.y, 0, dimMin.z, 0 }),
            GetOffsetInWorldCoords(pos, rot, fwd, {  dimMax.x, 0, -dimMax.y, 0, dimMax.z, 0 })
        };
    }

    void DrawModelExtents(Hash model, Vector3 pos, Vector3 rot, int r, int g, int b) {
        Vector3 dimMin, dimMax;
        MISC::GET_MODEL_DIMENSIONS(model, &dimMin, &dimMax);
        SBoxPoints box = GetBoxPoints(pos, rot, RotationToDirection(rot), dimMin, dimMax);

        UI::DrawLine(box.Lfd, box.Rfd, r, g, b, 255); // Front low
        UI::DrawLine(box.Lfu, box.Rfu, r, g, b, 255); // Front high
        UI::DrawLine(box.Lfd, box.Lfu, r, g, b, 255); // Front left
        UI::DrawLine(box.Rfd, box.Rfu, r, g, b, 255); // Front right

        UI::DrawLine(box.Lrd, box.Rrd, r, g, b, 255); // Rear low
        UI::DrawLine(box.Lru, box.Rru, r, g, b, 255); // Rear high
        UI::DrawLine(box.Lrd, box.Lru, r, g, b, 255); // Rear left
        UI::DrawLine(box.Rrd, box.Rru, r, g, b, 255); // Rear right

        UI::DrawLine(box.Lfu, box.Lru, r, g, b, 255); // Left up
        UI::DrawLine(box.Rfu, box.Rru, r, g, b, 255); // Right up
        UI::DrawLine(box.Lfd, box.Lrd, r, g, b, 255); // Left down
        UI::DrawLine(box.Rfd, box.Rrd, r, g, b, 255); // Right down
    }

    bool Intersects(Entity a, Entity b) {
        Hash modelA = ENTITY::GET_ENTITY_MODEL(a);
        Hash modelB = ENTITY::GET_ENTITY_MODEL(b);

        Vector3 posA = ENTITY::GET_ENTITY_COORDS(a, true);
        Vector3 posB = ENTITY::GET_ENTITY_COORDS(b, true);

        Vector3 rotA = ENTITY::GET_ENTITY_ROTATION(a, 0);
        Vector3 rotB = ENTITY::GET_ENTITY_ROTATION(b, 0);

        rotA = {
            deg2rad(rotA.x), 0,
            deg2rad(rotA.y), 0,
            deg2rad(rotA.z), 0,
        };

        rotB = {
            deg2rad(rotB.x), 0,
            deg2rad(rotB.y), 0,
            deg2rad(rotB.z), 0,
        };

        Vector3 dimMinA, dimMaxA;
        MISC::GET_MODEL_DIMENSIONS(modelA, &dimMinA, &dimMaxA);
        Util::SBoxPoints boxA = Util::GetBoxPoints(posA, rotA, RotationToDirection(rotA), dimMinA, dimMaxA);

        Vector3 dimMinB, dimMaxB;
        MISC::GET_MODEL_DIMENSIONS(modelB, &dimMinB, &dimMaxB);
        Util::SBoxPoints boxB = Util::GetBoxPoints(posB, rotB, RotationToDirection(rotB), dimMinB, dimMaxB);

        bool intersected = false;

        intersected = intersected || Intersect(boxA.Lfd, boxA.Rfd, boxB.Lfd, boxB.Rfd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Rfd, boxB.Lrd, boxB.Rrd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Rfd, boxB.Lfd, boxB.Lrd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Rfd, boxB.Rfd, boxB.Rrd);

        intersected = intersected || Intersect(boxA.Lrd, boxA.Rrd, boxB.Lfd, boxB.Rfd);
        intersected = intersected || Intersect(boxA.Lrd, boxA.Rrd, boxB.Lrd, boxB.Rrd);
        intersected = intersected || Intersect(boxA.Lrd, boxA.Rrd, boxB.Lfd, boxB.Lrd);
        intersected = intersected || Intersect(boxA.Lrd, boxA.Rrd, boxB.Rfd, boxB.Rrd);

        intersected = intersected || Intersect(boxA.Lfd, boxA.Lrd, boxB.Lfd, boxB.Rfd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Lrd, boxB.Lrd, boxB.Rrd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Lrd, boxB.Lfd, boxB.Lrd);
        intersected = intersected || Intersect(boxA.Lfd, boxA.Lrd, boxB.Rfd, boxB.Rrd);

        intersected = intersected || Intersect(boxA.Rfd, boxA.Rrd, boxB.Lfd, boxB.Rfd);
        intersected = intersected || Intersect(boxA.Rfd, boxA.Rrd, boxB.Lrd, boxB.Rrd);
        intersected = intersected || Intersect(boxA.Rfd, boxA.Rrd, boxB.Lfd, boxB.Lrd);
        intersected = intersected || Intersect(boxA.Rfd, boxA.Rrd, boxB.Rfd, boxB.Rrd);

        return intersected;
    }

    Vehicle CreateVehicle(Hash model, VehicleModData* vehicleModData, Vector3 pos) {
        STREAMING::REQUEST_MODEL(model);
        auto startTime = GetTickCount64();

        while (!STREAMING::HAS_MODEL_LOADED(model)) {
            WAIT(0);
            if (GetTickCount64() > startTime + 5000) {
                // Couldn't load model
                WAIT(0);
                STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
                std::string msg = std::format("Error: Failed to load model 0x{:08X}.", model);
                UI::Notify(msg, false);
                return 0;
            }
        }

        Vehicle vehicle = VEHICLE::CREATE_VEHICLE(model,
            pos.x, pos.y, pos.z, 0, false, true, false);
        if (vehicleModData)
            VehicleModData::ApplyTo(vehicle, *vehicleModData);
        VEHICLE::SET_VEHICLE_DIRT_LEVEL(vehicle, 0.0f);
        return vehicle;
    }
}
