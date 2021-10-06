#pragma once
#include "../VehicleMod.hpp"
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
    Vehicle CreateVehicle(Hash model, VehicleModData* vehicleModData, Vector3 pos);
}

// https://alloc8or.re/gta5/doc/enums/ePedType.txt
enum ePedType {
    PED_TYPE_PLAYER_0,
    PED_TYPE_PLAYER_1,
    PED_TYPE_NETWORK_PLAYER,
    PED_TYPE_PLAYER_2,
    PED_TYPE_CIVMALE,
    PED_TYPE_CIVFEMALE,
    PED_TYPE_COP,
    PED_TYPE_GANG_ALBANIAN,
    PED_TYPE_GANG_BIKER_1,
    PED_TYPE_GANG_BIKER_2,
    PED_TYPE_GANG_ITALIAN,
    PED_TYPE_GANG_RUSSIAN,
    PED_TYPE_GANG_RUSSIAN_2,
    PED_TYPE_GANG_IRISH,
    PED_TYPE_GANG_JAMAICAN,
    PED_TYPE_GANG_AFRICAN_AMERICAN,
    PED_TYPE_GANG_KOREAN,
    PED_TYPE_GANG_CHINESE_JAPANESE,
    PED_TYPE_GANG_PUERTO_RICAN,
    PED_TYPE_DEALER,
    PED_TYPE_MEDIC,
    PED_TYPE_FIREMAN,
    PED_TYPE_CRIMINAL,
    PED_TYPE_BUM,
    PED_TYPE_PROSTITUTE,
    PED_TYPE_SPECIAL,
    PED_TYPE_MISSION,
    PED_TYPE_SWAT,
    PED_TYPE_ANIMAL,
    PED_TYPE_ARMY
};

// https://alloc8or.re/gta5/doc/enums/eKnockOffVehicle.txt
enum eKnockOffVehicle {
    KNOCKOFFVEHICLE_DEFAULT,
    KNOCKOFFVEHICLE_NEVER,
    KNOCKOFFVEHICLE_EASY,
    KNOCKOFFVEHICLE_HARD
};

enum EVehicleLightState : uint32_t {
    LightStateIndicatorLeft = 1 << 8,
    LightStateIndicatorRight = 1 << 9,
};

enum EIndicators {
    IndicatorRight = 0,
    IndicatorLeft = 1,
};
