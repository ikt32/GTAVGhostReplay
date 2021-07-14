#pragma once
#include "Math.hpp"
#include "UI.hpp"
#include <inc/natives.h>

namespace Util {
    struct SBoxPoints {
        Vector3 Lfd;
        Vector3 Lfu;
        Vector3 Rfd;
        Vector3 Rfu;
        Vector3 Lrd;
        Vector3 Lru;
        Vector3 Rrd;
        Vector3 Rru;
    };

    inline SBoxPoints GetBoxPoints(Vector3 pos, Vector3 rot, Vector3 fwd, Vector3 dimMax, Vector3 dimMin) {
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

    inline void DrawModelExtents(Hash model, Vector3 pos, Vector3 rot, int r, int g, int b) {
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

    inline bool VehicleAvailable(Vehicle vehicle, Ped playerPed, bool checkSeat) {
        bool seatOk = true;
        if (checkSeat)
            seatOk = playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, 0);

        return vehicle != 0 &&
            ENTITY::DOES_ENTITY_EXIST(vehicle) &&
            PED::IS_PED_IN_VEHICLE(playerPed, vehicle, false) &&
            seatOk;
    }
}
