#include <Windows.h>
#include "../src/pipe.h"
int main(){crimson::live_items::Queue queue(GetCurrentProcessId(),GetTickCount64());queue.observe(GetTickCount64());crimson::live_items::serve(queue);return 1;}
