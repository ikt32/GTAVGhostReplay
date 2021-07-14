#pragma once
#include <inc/types.h>
#include <cmath>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

// Vector3 but double and no padding. 
struct V3D {
    V3D(Vector3 v3f)
        : x(v3f.x), y(v3f.y), z(v3f.z) { }
    V3D(double x, double y, double z)
        : x(x), y(y), z(z) { }
    V3D() = default;
    Vector3 to_v3f() {
        Vector3 v{
            static_cast<float>(x), 0,
            static_cast<float>(y), 0,
            static_cast<float>(z), 0
        };
        return v;
    }
    double x;
    double y;
    double z;
};

template <typename T>
constexpr T sgn(T val) {
    return static_cast<T>((T{} < val) - (val < T{}));
}

template<typename T, typename A>
T avg(std::vector<T, A> const& vec) {
    T average{};
    for (auto elem : vec)
        average += elem;
    return average / static_cast<T>(vec.size());
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
constexpr T rad2deg(T rad) {
    return static_cast<T>(static_cast<double>(rad) * (180.0 / M_PI));
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
constexpr T deg2rad(T deg) {
    return static_cast<T>(static_cast<double>(deg) * M_PI / 180.0);
}

template <typename T>
T map(T x, T in_min, T in_max, T out_min, T out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

namespace Math {
    template <typename T>
    bool Near(T a, T b, T deviation) {
        return (a > b - deviation && a < b + deviation);
    }
}

template <typename T, typename T2>
T lerp(T a, T b, T2 f) {
    return a + f * (b - a);
}

template <typename T>
T wrap(T x, T min, T max) {
    T delta = (max - min);
    x = fmod(x + max, delta);
    if (x < 0)
        x += delta;
    return x + min;
}

// Loops back on lim, centered around 0 (e.g. -175.5 - 1 -> +175.5)
template <typename T, typename T2>
T lerp(T a, T b, T2 f, T2 min, T2 max) {
    return a + f * wrap(b - a, min, max);
}

inline Vector3 lerp(Vector3 a, Vector3 b, float f, float min, float max) {
    return Vector3{
        lerp(a.x, b.x, f, min, max), 0,
        lerp(a.y, b.y, f, min, max), 0,
        lerp(a.z, b.z, f, min, max), 0,
    };
}

template<typename T>
concept TVec3 = requires(T t) { t.z; };

template <TVec3 Vector3T>
auto Length(Vector3T vec) {
    return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

template <TVec3 Vector3T>
auto Distance(Vector3T vec1, Vector3T vec2) {
    Vector3T distance = vec1 - vec2;
    return Length(distance);
}

template <TVec3 Vector3T>
auto Dot(Vector3T a, Vector3T b)  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template <TVec3 Vector3T>
Vector3T Cross(Vector3T left, Vector3T right) {
    Vector3T result{};
    result.x = left.y * right.z - left.z * right.y;
    result.y = left.z * right.x - left.x * right.z;
    result.z = left.x * right.y - left.y * right.x;
    return result;
}

template <TVec3 Vector3T>
Vector3T operator + (Vector3T left, Vector3T right) {
    Vector3T v{};
    v.x = left.x + right.x;
    v.y = left.y + right.y;
    v.z = left.z + right.z;
    return v;
}

template <TVec3 Vector3T>
Vector3T operator - (Vector3T left, Vector3T right) {
    Vector3T v{};
    v.x = left.x - right.x;
    v.y = left.y - right.y;
    v.z = left.z - right.z;
    return v;
}

template <TVec3 Vector3T>
Vector3T operator * (Vector3T value, decltype(value.x) scale) {
    Vector3T v{};
    v.x = value.x * scale;
    v.y = value.y * scale;
    v.z = value.z * scale;
    return v;
}

template <TVec3 Vector3T>
Vector3T operator * (decltype(std::declval<Vector3T>().x) scale, Vector3T vec) {
    return vec * scale;
}

template <TVec3 Vector3T>
Vector3T Normalize(Vector3T vec) {
    Vector3T vector = {};
    auto length = Length(vec);

    if (length != static_cast<decltype(vec.x)>(0.0)) {
        vector.x = vec.x / length;
        vector.y = vec.y / length;
        vector.z = vec.z / length;
    }

    return vector;
}

template <TVec3 Vector3T>
Vector3T GetOffsetInWorldCoords(Vector3T position, Vector3T rotation, Vector3T forward, Vector3T offset) {
    auto num1 = cos(rotation.y);
    auto x = num1 * cos(-rotation.z);
    auto y = num1 * sin(rotation.z);
    auto z = sin(-rotation.y);
    Vector3 right = { x, 0, y, 0, z, 0 };
    Vector3 up = Cross(right, forward);
    return position + (right * offset.x) + (forward * offset.y) + (up * offset.z);
}

// degrees
template <typename T>
T GetAngleBetween(T h1, T h2, T separation) {
    T diff = abs(h1 - h2);
    if (diff < separation)
        return (separation - diff) / separation;
    if (abs(diff - static_cast<T>(360.0)) < separation)
        return (separation - abs(diff - static_cast<T>(360.0))) / separation;
    return separation;
}

template <TVec3 Vector3T>
auto GetAngleBetween(Vector3T a, Vector3T b) {
    Vector3T normal{};
    normal.z = static_cast < decltype(a.x) >(1.0);
    auto angle = acos(Dot(a, b) / (Length(a) * Length(b)));
    if (Dot(normal, Cross(a, b)) < 0.0f)
        angle = -angle;
    return angle;
}

template <TVec3 Vector3T>
Vector3T RotationToDirection(Vector3T rot) {
    auto rotZ = (rot.z);
    auto rotX = (rot.x);
    auto multXY = abs(cos(rotX));
    Vector3T v{};
    v.x = -sin(rotZ) * multXY;
    v.y = cos(rotZ) * multXY;
    v.z = sin(rotX);
    return v;
}

template <TVec3 Vector3T>
Vector3T RelativeRightVector(Vector3T rot) {
    auto num = cos(rot.y);
    Vector3T v{};
    v.x = cos(-rot.z) * num;
    v.y = sin(rot.z) * num;
    v.z = sin(-rot.y);
    return v;
}

template <TVec3 Vector3T>
inline Vector3T GetRelativeOffsetGivenWorldCoords(Vector3T position1, Vector3T position2, Vector3T rot) {
    Vector3T right = RelativeRightVector(rot);
    Vector3T forward = RotationToDirection(rot);

    Vector3T v{};
    v.x = -1 * Dot(right, (position1 - position2));
    v.y = -1 * Dot(forward, (position1 - position2));
    v.z = position2.z - position1.z;
    return v;
}

inline bool operator==(const Vector3& lhs, const Vector3& rhs) {
    return lhs.x == rhs.x &&
           lhs.y == rhs.y &&
           lhs.z == rhs.z;
}

inline bool operator!=(const Vector3& lhs, const Vector3& rhs) {
    return !(lhs == rhs);
}

inline float GetAngleABC(Vector3 a, Vector3 b, Vector3 c)
{
    Vector3 ab = { b.x - a.x, 0, b.y - a.y };
    Vector3 cb = { b.x - c.x, 0, b.y - c.y };

    float dot = (ab.x * cb.x + ab.y * cb.y); // dot product
    float cross = (ab.x * cb.y - ab.y * cb.x); // cross product

    float alpha = atan2(cross, dot);

    return alpha;// (int)floor(alpha * 180. / pi + 0.5);
}

inline Vector3 GetPerpendicular(Vector3 a, Vector3 b, float length, bool counterclockwise) {
    Vector3 ab = Normalize(b - a);
    Vector3 abCw{};
    if (counterclockwise) {
        abCw.x = -ab.y;
        abCw.y = ab.x;
    }
    else {
        abCw.x = ab.y;
        abCw.y = -ab.x;
    }
    return a + abCw * length;
}

inline bool ccw(Vector3 A, Vector3 B, Vector3 C) {
    return (C.y - A.y) * (B.x - A.x) > (B.y - A.y) * (C.x - A.x);
}

// Return true if line segments AB and CD intersect
// Only X,Y plane is checked
inline bool Intersect(Vector3 A, Vector3 B, Vector3 C, Vector3 D) {
    return ccw(A, C, D) != ccw(B, C, D) && ccw(A, B, C) != ccw(A, B, D);
}
