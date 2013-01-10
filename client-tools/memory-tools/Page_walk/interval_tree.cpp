#include <boost/icl/interval_map.hpp>
#include <cstdio>

using namespace boost::icl;

typedef unsigned long long tADDR;

typedef int  tRECORD;


void Print(const interval_set<tADDR>& intervals)
{
	interval_set<tADDR>::const_iterator it;

	printf("Printing interval...\n");

 	for ( it = intervals.begin(); it!= intervals.end(); ++it) {
         printf(" [%llu,%llu)\n", it->lower(), it->upper());
    }

	printf("\n");
}

int main()
{
    interval_set<tADDR> swap_range;

/*    swap_range.add(  interval<tADDR>::right_open(0,4) );
 	swap_range +=  interval<tADDR>::right_open(9,40) ;

	Print(swap_range);
	swap_range +=  interval<tADDR>::right_open(4,9) ;
	
	Print(swap_range);

	swap_range -=  interval<tADDR>::right_open(38,39) ;
	Print(swap_range);*/

	interval_map<unsigned long, char> map;
	map += interval<unsigned long, char>::right_open(1,1);

	

    return 0;
}

