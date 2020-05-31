#pragma once

typedef struct {
    BfxList procList;
} _AtExitManager;

_AtExitManager* _newAtExitManager();
void _deleteAtExitManager(_AtExitManager* manager);

void _execAtExits(_AtExitManager* manager);
int _atExit(BfxThreadExitCallBack proc, BfxUserData userData);