#pragma once


#include <string>


extern int		GetVariable(const std::string& input, const std::string& var);
extern int		GetCpmaConfigStringInt(const char* var, const char* str); // Gets the integer value of a CPMA config string value (the name has only 2 characters).
extern bool		GetVariable(const std::string& input, const std::string& var, int* value);
extern bool		TryGetVariable(int* varValue, const std::string& input, const std::string& varName);
extern void		GetVariable(std::string& var, const std::string& input, const std::string& cvar);
extern void		ChangeVariable(const std::string& input, const std::string& cvar, const std::string newVal, std::string& output);
extern void		ChangeVariable(const std::string& input, const std::string& cvar, int val, std::string& output);
extern int		ConvertPowerUpFlagsToValue(int flags); // Will keep/return the first power-up it finds.
extern void		ReadScore(const char* scoreString, int* scoreValue); // Reads the score, sets it to -9999 when it fails.