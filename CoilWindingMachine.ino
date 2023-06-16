#include <AccelStepper.h>
#include <myDebug.h>

// for red A498 driver

#define STEPS_PER 200
#define dirPin A3
#define stepPin A4
#define enablePin A5

#define motorInterfaceType 1

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);


int revs = 10;
int speed = 200;
bool locked = 0;
int turns = 0;
	// only used to set steppers current position
	// displayed turns is function of this

bool running = 0;
long last_pos = 0;
long run_to = 0;


void setup()
{
	Serial.begin(115200);
	display(0,"coil_winder.ino started",0);
	stepper.setMaxSpeed(1000);
	stepper.disableOutputs();
	stepper.setEnablePin(enablePin);
	stepper.setPinsInverted(false,false,true);	// third is enable
	stepper.setCurrentPosition(0);
}





static int in_mult = 1;
static int in_value = 0;
static int in_min = 0;
static int in_max = 255;
static const char *in_what = "";
static int *in_rslt = 0;
static bool in_started = 0;


static void startInt(int *rslt, const char *what, int min, int max)
{
	in_mult = 0;
	in_value = 0;
	in_min = min;
	in_max = max;
	in_what = what;
	in_rslt = rslt;
	in_started = 1;
}


static bool handleInt(int c)
{
	if (!in_started)
		return false;

	if (c == 10 || c == 13)
	{
		if (in_mult)
		{
			in_value = in_mult  * in_value;
			if (in_value < in_min)
			{
				display(0,"%s must be <= %d",in_what,in_min);
				in_value = in_min;
			}
			else if (in_value > in_max)
			{
				display(0,"%s must be >= %d",in_what,in_max);
				in_value = in_max;
			}
			*in_rslt = in_value;
		}
		display(0,"%s %d",in_what,*in_rslt);
		if (!strcmp(in_what,"TURNS"))
		{
			stepper.setCurrentPosition(turns * STEPS_PER);
		}
		in_started = 0;
	}
	else if (c == '-')
	{
		in_mult = in_mult == -1 ? 1 : -1;
	}
	else if (c >= '0' && c <= '9')
	{
		if (!in_mult) in_mult = 1;
		in_value = (in_value * 10) + (c - '0');
	}

	return true;
}




void stop()
{
	if (!running)
		return;

	stepper.stop();
	stepper.disableOutputs();
	long show_turns = stepper.currentPosition();
	show_turns = show_turns / STEPS_PER;
	display(0,"STOPPED turns=%d",show_turns);
	running = 0;
	locked = 0;
}


void loop()
{
	if (Serial.available())
	{
		int c = Serial.read();
		if (!handleInt(c))
		{
			if (c == 'l')
			{
				stop();
				locked = !locked;
				if (locked)
				{
					stepper.setSpeed(0);
					stepper.enableOutputs();
				}
				else
					stepper.disableOutputs();
				display(0,"LOCKED(%d)",locked);
			}

			else if (c == ' ')
			{
				if (!running)
				{
					display(0,"RUN revs(%d) speed(%d)",revs, speed);
					stepper.setSpeed(speed * (revs > 0 ? 1 : revs < 0 ? -1 : 0));

					run_to = revs;
					run_to *= STEPS_PER;
					run_to += stepper.currentPosition();

					stepper.enableOutputs();
					locked = 1;
					running = 1;
				}
				else
				{
					stop();
				}
			}
			else if (c == 'r')
			{
				stop();
				startInt(&revs,"REVS",-1000,1000);
			}
			else if (c == 's')
			{
				stop();
				startInt(&speed,"SPEED",20,1000);
			}
			else if (c == 't')
			{
				stop();
				startInt(&turns,"TURNS",-1000,4000);
			}
		}
	}

	if (running)
	{
		long pos = stepper.currentPosition();
		if (last_pos != pos)
		{
			last_pos = pos;
			if (pos % (STEPS_PER * 10) == 0)
			{
				long show_turns = pos / STEPS_PER;
				display(0,"AT %d",show_turns);
			}
			if (revs > 0 && pos >= run_to)
				stop();
			else if (revs < 0 && pos <= run_to)
				stop();
			else if (revs == 0)
				stop();
		}
	}
	if (running)
		stepper.runSpeed();

}
