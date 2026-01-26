#pragma once

bool initPMS();
void pollPMS();
bool readPMS(std::array<uint16_t, 3> &out);