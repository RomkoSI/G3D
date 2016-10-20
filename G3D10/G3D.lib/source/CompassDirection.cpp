#include "G3D/CompassDirection.h"
#include "G3D/Matrix3.h"
#include "G3D/Vector3.h"
#include "G3D/Any.h"

namespace G3D {

CompassDelta::CompassDelta(const Any& a) {
    a.verifyType(Any::ARRAY);
    a.verifyName("CompassDelta", "CompassBearing", "degrees");
    a.verifySize(1);
    m_angleDegrees = a[1];
}


Any CompassDelta::toAny() const {
    Any a(Any::ARRAY, "CompassDelta");
    a.append(m_angleDegrees);
    return a;
}


CompassDirection::CompassDirection(const Any& a) {
    a.verifyType(Any::ARRAY);
    a.verifyName("CompassDirection", "degrees");
    a.verifySize(1);
    m_angleDegrees = a[0];
}


Any CompassDirection::toAny() const {
    Any a(Any::ARRAY, "CompassDirection");
    a.append(m_angleDegrees);
    return a;
}

CompassDirection::operator Vector3() const {
    const float a = zxRadians();
    return Vector3(sin(a), 0, cos(a));
}


Matrix3 CompassDirection::toHeadingMatrix3() const {
    return Matrix3::fromAxisAngle(Vector3::unitY(), zxRadians() + pif());
}


static const String NAME_TABLE[33 * 2] = {
"North", "N",
"North by east", "NbE",
"North-northeast", "NNE",
"Northeast by north", "NEbN",
"Northeast", "NE",
"Northeast by east", "NEbE",
"East-northeast", "ENE",
"East by north", "EbN",
"East", "E",
"East by south", "EbS",
"East-southeast", "ESE",
"Southeast by east", "SEbE",
"Southeast", "SE",
"Southeast by south", "SEbS",
"South-southeast", "SSE",
"South by east", "SbE",
"South", "S",
"South by west", "SbW",
"South-southwest", "SSW",
"Southwest by south", "SWbS",
"Southwest", "SW",
"Southwest by west", "SWbW",
"West-southwest", "WSW",
"West by south", "WbS",
"West", "W",
"West by north", "WbN",
"West-northwest", "WNW",
"Northwest by west", "NWbW",
"Northwest", "NW",
"Northwest by north", "NWbN",
"North-northwest", "NNW",
"North by west", "NbW",
"North","N"    };

const String& CompassDirection::nearestCompassPointName() const {
    // Change names every 12.5 degrees, offset slightly
    const int i = iFloor(wrap(m_angleDegrees + (360 / 64.0f), 360) / 32);
    debugAssertM(i >= 0 && i < 32, "Index out of bounds");
    return NAME_TABLE[2 * i];
}

const String& CompassDirection::nearestCompassPointAbbreviation() const {
    // Change names every 12.5 degrees, offset slightly
    const int i = iFloor(wrap(m_angleDegrees + (360 / 64.0f), 360) / 32);
    debugAssertM(i >= 0 && i < 32, "Index out of bounds");
    return NAME_TABLE[2 * i + 1];
}

} // G3D
