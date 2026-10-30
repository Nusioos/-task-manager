#include"Colors.hpp"

ImU32 GetColorById(int id) {
    switch (id) {
    case 1:  return IM_COL32(30, 130, 30, 255);    // Medium-Dark Green
    case 2:  return IM_COL32(30, 80, 160, 255);    // Medium-Dark Blue
    case 3:  return IM_COL32(160, 40, 40, 255);    // Medium-Dark Red
    case 4:  return IM_COL32(150, 140, 30, 255);   // Dark Yellow / Gold
    case 5:  return IM_COL32(130, 40, 130, 255);   // Medium-Dark Magenta
    case 6:  return IM_COL32(170, 90, 20, 255);    // Dark Orange / Burnt Orange
    case 7:  return IM_COL32(90, 30, 120, 255);    // Medium-Dark Purple
    case 8:  return IM_COL32(160, 70, 100, 255);   // Dark Pink / Rose
    case 9:  return IM_COL32(110, 150, 30, 255);   // Muted Lime
    case 10: return IM_COL32(110, 60, 20, 255);    // Warm Brown
    case 11: return IM_COL32(160, 160, 160, 255);  // Soft White / Off-White
    case 12: return IM_COL32(90, 90, 90, 255);     // Mid Gray
    case 13: return IM_COL32(28, 28, 28, 255);     // Dark Charcoal
    case 14: return IM_COL32(255, 255, 255, 255);
    default: return IM_COL32(45, 45, 45, 255);     // Dark Gray
    }
}
