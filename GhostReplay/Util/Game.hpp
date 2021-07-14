#pragma once
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

    bool VehicleAvailable(Vehicle vehicle, Ped playerPed, bool checkSeat);
    SBoxPoints GetBoxPoints(Vector3 pos, Vector3 rot, Vector3 fwd, Vector3 dimMax, Vector3 dimMin);
    void DrawModelExtents(Hash model, Vector3 pos, Vector3 rot, int r, int g, int b);
    bool Intersects(Entity a, Entity b);
}
