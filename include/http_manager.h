#pragma once

extern const char *API_URL;

bool registerHttpsRequest();
bool checkModuleRegistrationRequest(std::string moduleToken);
bool handleAuthInitialization();
