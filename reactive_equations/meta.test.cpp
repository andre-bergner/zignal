#include "../flowz/demangle.h"
#include "meta.h"

#include <iostream>


   template <typename S, typename T>
   struct insert_set : meta::insert<S,T> {};

   template <typename S, typename T>
   using insert_set_t = typename insert_set<S,T>::type;

int main()
{
   using namespace meta;
   using namespace std;

   using s0 = type_set<>;
   using s1 = insert_t< s0, int >;
   using s2 = insert_t< s1, int >;
   using s3 = insert_t< s2, char >;

   cout << type_name<s0>() << endl;
   cout << type_name<s1>() << endl;
   cout << type_name<s2>() << endl;
   cout << type_name<s3>() << endl;


   using m0 = type_map<>;
   using m1 = insert_t< m0, int, float>;
   using m2 = insert_t< m1, int, bool>;
   using m3 = insert_t< m2, char, bool >;

   cout << "--------------------------------------" << endl;
   cout << type_name<m0>() << endl;
   cout << type_name<m1>() << endl;
   cout << type_name<m2>() << endl;
   cout << type_name<m3>() << endl;
   cout << "--------------------------------------" << endl;
   cout << type_name< replace_t<m3,char,float> >() << endl;
   cout << type_name< replace_t<m3,int,bool> >() << endl;
   cout << type_name< replace_t<type_map<>,int,bool> >() << endl;
   cout << "--------------------------------------" << endl;
   cout << type_name< force_insert_t<m3,char,float> >() << endl;
   cout << type_name< force_insert_t<m3,int,bool> >() << endl;
   cout << type_name< force_insert_t<type_map<>,int,bool> >() << endl;
   cout << "--------------------------------------" << endl;
   cout << "type_at_t<map>:\n";
   cout << type_name< type_at_t<m3,char> >() << endl;
   cout << type_name< type_at_t<m3,bool> >() << endl;

   cout << "--------------------------------------" << endl;
   cout << index_of_v<m3,int> << endl;
   cout << index_of_v<m3,char> << endl;

   cout << "--------------------------------------" << endl;
   cout << index_of_v<type_list<int,char,bool>,int> << endl;
   cout << index_of_v<type_list<int,char,bool>,char> << endl;
   cout << index_of_v<type_list<int,char,bool>,bool> << endl;

   cout << "--------------------------------------" << endl;
   cout << "type_set:\n";
   static_assert(std::is_same_v<
      insert_t<type_set<char, float>, char>,
      type_set<char, float>
   >);

   static_assert(std::is_same_v<
      force_insert_t<type_set<char, float>, float>,
      type_set<char, float>
   >);

   static_assert(std::is_same_v<
      force_insert_t<type_set<char, float>, char>,
      type_set<float, char>
   >);
   cout << type_name< force_insert_t<type_set<char, float>, bool> >() << endl;
   cout << type_name< force_insert_t<type_set<>, bool> >() << endl;
   cout << type_name< force_insert_t<type_set<char, float>, char> >() << endl;



   cout << "--------------------------------------" << endl;

   using S1 = type_set< int, char >;
   using S2 = type_set< bool, float >;

   cout << type_name< fold_t<insert_t, S1, S2> >() << endl;






}
