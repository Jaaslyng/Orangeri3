/******************************************************************
 Created with PROGRAMINO IDE for Arduino - 10.07.2017 20:23:39
 Libraries   :
 Author      :
 Description :
******************************************************************/

#ifndef MyStat_h
#define MyStat_h

#include <iostream>
using namespace std;

// write your header code here ...
class MyStat {
 int count;
 float min;
 float max;
 float sum;
 float mean; 

 public:
 
 void AddData(float newdata) 
 {

 //cout << "AddData " << newdata << endl;
 
 sum = sum + newdata;
  count = count + 1;
  if (newdata < min)
    {min = newdata;}
  if (newdata > max)
    {max = newdata;}
  };
 
  float GetMean()
 {
  
 cout << ("GetMean"); 
 if (count > 0)
      {mean = sum/count;
      return mean;}
   
    if (count = 0)
      {return -9999;}
     
 cout << ("GetMean ");
  cout << ("Count ");
  cout << (count);
  cout << (" sum ");
  cout << (sum);
  cout << (" mean ");
  cout << (mean);
 
 
 };
 
  float GetMin()
 {
    return min;
 };
 
 float GetMax()
 {
    return max;
 };
 
 float GetSum()
 {
    return sum;
 };
 
 void clear()
 {
 sum = 0;
 count = 0;
 min = 99999999;
 max = -9999999;
 };
 
};


#endif
