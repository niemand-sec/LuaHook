# LuaHook

This version has been adapted to work perfectly for any game. Just a pattern and a mask need to be provided. This pattern will be used to locate the target function to hook and place the hook there.

Detour has been replaced by MinHook.

# Pattern and Mask Example

strcpy(test, "\x48\x8B\x41\x18\x48\x2B\x41\x10\x48\xc1\xf8\x03\xc3");
strcpy(test2, "xxxxxxxxxxxxx");

"?" can be used as wildcard instead of X, which is for fixed character.


# Kudos:

Great tutorial about signature scanning - https://guidedhacking.com/threads/c-signature-scan-pattern-scanning-tutorial.3981/
