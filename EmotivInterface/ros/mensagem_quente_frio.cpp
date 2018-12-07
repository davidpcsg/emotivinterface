#include "ros/ros.h"
#include "std_msgs/String.h" 
#include <sstream>
#include <signal.h>
#include <termios.h>
#include <stdio.h>

#define KEYCODE_Q 0x71 
#define KEYCODE_F 0x66
#define KEYCODE_S 0x73

int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{	
  tcsetattr(kfd, TCSANOW, &cooked);
  ros::shutdown();
  exit(0);
}

int main(int argc, char **argv)
{

	ros::init(argc,argv,"mensagem_quente_frio");
	ros::NodeHandle n;
	ros::Publisher chatter_pub = n.advertise<std_msgs::String>("temperatura", 10);
	ros::Rate loop_rate(10);
	
	char c;
	int i;
	
	while (ros::ok())
	{
		signal(SIGINT,quit);           
		
  		tcgetattr(kfd, &cooked);
  		memcpy(&raw, &cooked, sizeof(struct termios));
  		raw.c_lflag &=~ (ICANON | ECHO);                    
  		raw.c_cc[VEOL] = 1;
  		raw.c_cc[VEOF] = 2;
  		tcsetattr(kfd, TCSANOW, &raw);
		
		std_msgs::String msg;
		std::stringstream ss;

	    read(kfd, &c, 1);
         switch(c)
              {
  
                case KEYCODE_Q:
				   ss <<"Q";
		           msg.data = ss.str();	  
                   break;
                case KEYCODE_F:
				   ss <<"F" ;
		           msg.data = ss.str();						  
                   break;
                case KEYCODE_S:
				   ss <<"S" ;
		           msg.data = ss.str();						  
                   break;					  	  
				default:
				   ss <<"Z";
		           msg.data = ss.str();	
              }
	

		ROS_INFO ("%s", msg.data.c_str());		
		chatter_pub.publish(msg);
		ros::spinOnce();
		loop_rate.sleep();
	}
	
	return 0;

}
