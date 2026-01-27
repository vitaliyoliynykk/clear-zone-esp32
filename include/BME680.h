#pragma once

void initBME();
bool readGas(std::pair<uint32_t, float> &out);
uint64_t getBmeUptimeSec();
bool getIsBMEReady();