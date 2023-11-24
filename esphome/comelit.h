#include <bitset>
#include "esphome.h"
using namespace std;

std::vector<int> comelit_encode(int address, int code)
{
    std::vector<int> out;
    bitset<6> Code(code);
    bitset<8> Addr(address);

    int cnt = 0;

    out.push_back(3000);
    out.push_back(-16000);

    for (int i = 0; i < 6; i++)
    {
        out.push_back(3000);
        if (Code.test(i))
        {
            out.push_back(-6000);
            cnt++;
        }
        else
        {
            out.push_back(-3000);
        }
    }

    for (int i = 0; i < 8; i++)
    {
        out.push_back(3000);
        if (Addr.test(i))
        {
            out.push_back(-6000);
            cnt++;
        }
        else
        {
            out.push_back(-3000);
        }
    }

    bitset<4> Check(cnt);
    ESP_LOGD("DEBUG", "%i", cnt);

    for (int i = 0; i < 4; i++)
    {
        out.push_back(3000);
        if (Check.test(i))
        {
            out.push_back(-6000);
        }
        else
        {
            out.push_back(-3000);
        }
    }

    out.push_back(3000);
    out.push_back(-20000);
    return out;
}
