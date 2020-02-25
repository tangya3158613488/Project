#include "Sakura.hpp"
using namespace std;
int main()
{
   // Robot rb;
   // string s = rb.Talk("你好");
   // cout << s << endl;
 //  string str;
  // volatile bool quit = false;
  // while(!quit)
  // {
  //     cout << "我# ";
  //     cin >> str;
  //     string s = rb.Talk(str);
  //     cout<< "Sakura# " << s <<  endl;
  // }
     Sakura *s = new Sakura();
     if(!s->LoadEtc())
     {
         return 1;
     }
     s->Run();
    return 0;
}
