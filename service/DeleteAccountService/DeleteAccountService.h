#pragma once
#include <nlohmann/json.hpp>
using json = nlohmann::json;
class DeleteAccountService{
public:
    static json removeAccount(const json& js);
};