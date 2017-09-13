#include <mutex>

static std::once_flag onceFlag;

static void initCode()
{ }

int main()
{
    std::call_once(onceFlag, initCode);
}
