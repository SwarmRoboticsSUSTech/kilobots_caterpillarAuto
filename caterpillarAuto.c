#include "kilolib.h"

// Gradient.
#define SEED_ID 0
#define GRADIENT_MAX 255

// Time or duration encoding.
// 32 * n, n seconds.
#define TIME_CHECK_MAXER 32 * 5 
#define TIME_CHECK_MINOR 32 * 4
#define TIME_LAST_GRADIENT 32 * 5 
#define TIME_LAST_MOTION_UPDATE 32 * 0.7

// Distance encoding.
// mm.
#define DISTANCE_GRADIENT 100  
#define DISTANCE_MIN 33 
#define DISTANCE_MAX 100 
#define DISTANCE_MOVE 42 
#define DISTANCE_COLLIDE 40 
#define DISTANCE_STOP 50 

// Motion encoding.
#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3
#define MOVE 4
#define COMPLETED 5

// Flag encoding.
#define FORMED_OK 1
#define FORMED_NO 0
#define YES 1
#define NO 0
#define UPDATE 1
#define UNUPDATE 0

// Logic expression encoding.
#define NEARER (distance_to_motivated < distance_to_motivated_best)
#define EQUAL (distance_to_motivated == distance_to_motivated_best)
#define FARER (distance_to_motivated > distance_to_motivated_best)
#define INLINE (distance_line <= distance_line_best)
#define OUTLINE (distance_line > distance_line_best)
#define NEARER_INLINE (NEARER && INLINE)
#define NEARER_OUTLINE (NEARER && OUTLINE)
#define EQUAL_INLINE (EQUAL && INLINE)
#define EQUAL_OUTLINE (EQUAL && OUTLINE)
#define FARER_INLINE (FARER && INLINE)
#define FARER_OUTLINE (FARER && OUTLINE)

// Logic encoding.
#define LOGIC_NEARER 0
#define LOGIC_EQUAL 1
#define LOGIC_FARER 2
#define LOGIC_INLINE 3
#define LOGIC_OUTLINE 4


// Gradient.
int own_gradient = GRADIENT_MAX;
int received_gradient = 0;

// Time or duration
uint32_t last_gradient_anchored;
uint32_t last_found_maxer;
uint32_t last_found_minor;
uint32_t last_motion_update;

// Distance.
// motivator: my direct follower.
// motivated: my direct leader.
int distance = DISTANCE_MAX;
int distance_to_motivator = DISTANCE_MAX;
int distance_to_motivator_pair = DISTANCE_MAX;
int distance_to_motivator_best = DISTANCE_MAX;
int distance_to_motivated = DISTANCE_MAX;
int distance_to_motivated_best = DISTANCE_MAX;
int distance_line = DISTANCE_MAX;
int distance_line_best = DISTANCE_MAX;

// Motion.
int current_motion = FORWARD;
int next_motion = FORWARD;
int num_stop = 0;
int my_fault = YES;
int offspring = FORWARD;

// Logic.
int state_motivated = STOP;
int state_motivator = STOP;
int state_myself = STOP;
int flag_maxest = NO;
int flag_minor = NO;
int formed_state = FORMED_NO;

int last_logic_1 = LOGIC_EQUAL;
int last_logic_2 = LOGIC_INLINE;

int update_distance_to_motivated = UNUPDATE;
int update_distance_to_motivator = UNUPDATE;
int update_state_motivated = UNUPDATE;
int update_state_motivator = UNUPDATE;

// Message.
message_t message;


// Generate 0 or 1 randomly
int randBinary(){
    // Generate an 8-bit random number (between 0 and 2^8 - 1 = 255).
    int random_number = rand_hard();

    // Compute the remainder of random_number when divided by 2.
    // This gives a new random number in the set {0, 1}.
    int random_direction = (random_number % 2);

    return random_direction;
}


// Function to set led states
void set_led()
{
    // Set the LED color based on the gradient.
    switch (own_gradient) {
        case 0:
            set_color(RGB(1, 1, 1)); // White
            break;
        case 1:
            set_color(RGB(1, 0, 0)); // Red
            break;
        case 2:
            set_color(RGB(0, 1, 0)); // Green
            break;
        case 3:
            set_color(RGB(0, 0, 1)); // Blue
            break;
        case 4:
            set_color(RGB(1, 1, 0)); // Yellow
            break;
        default:
            set_color(RGB(1, 0, 1)); // Magneta
            break;
    }
    //  set_color(RGB(0, 1, 1));
}


// Function to handle motion.
void set_motion(int new_motion)
{
    // Only take an an action if the motion is being changed.
    if (current_motion != new_motion)
    {
        current_motion = new_motion;

        if (current_motion == STOP)
        {
            set_color(RGB(0, 0, 0));
            set_motors(0, 0);
        }
        else if (current_motion == FORWARD)
        {
	    //set_color(RGB(1, 0, 0));
            spinup_motors();
            set_motors(kilo_straight_left, kilo_straight_right);
        }
        else if (current_motion == LEFT)
        {
	    //set_color(RGB(0, 1, 0));
            spinup_motors();
            set_motors(kilo_turn_left, 0);
        }
        else if (current_motion == RIGHT)
        {
	    //set_color(RGB(0, 0, 1));
            spinup_motors();
            set_motors(0, kilo_turn_right);
        }
    }
}


void setup()
{
    //If the robot is the seed, its gradient should be 0: overwrite the
    // previously set value of GRADIENT_MAX.
    if (kilo_uid == SEED_ID)
    {
        own_gradient = 0;
	distance_to_motivator = DISTANCE_COLLIDE;
	update_distance_to_motivator = UPDATE;
	state_motivator = COMPLETED;
	update_state_motivator = UPDATE;
	flag_minor = YES;
    }

    // Set the transmission message.
    message.type = NORMAL;
    message.data[0] = own_gradient;
    // Sequence has not been formed completely.
    message.data[1] = formed_state;
    message.data[2] = state_motivator;
    message.data[3] = distance_to_motivator;
    message.crc = message_crc(&message);
}


// If I am lost from all the others in the world, there should be mechnism
// allowing me to find this serious thing since I am gregarious.
void check_own_gradient() {
    // If no neighbors detected within TIME_LAST_GRADIENT seconds
    // then sleep waiting for be activated.
    if ( (kilo_uid != SEED_ID) && (kilo_ticks > (last_found_minor + TIME_LAST_GRADIENT)) && (own_gradient < GRADIENT_MAX))
    {
        own_gradient = GRADIENT_MAX;
	formed_state = FORMED_NO;
    }
}


// Convert the current motion to its opposite one.
int opposite_move(int offspring)
{
	set_color(RGB(1, 1, 1));
	int next_motion = offspring;
	switch (offspring)
	{
		case LEFT:
			next_motion = RIGHT;
			break;
		case RIGHT:
			next_motion = LEFT;
			break;
		case FORWARD:
			if (randBinary() == 1)
			{
				next_motion = LEFT;
			}
			else
			{
				next_motion = RIGHT;
			}
			break;
		default:
			break;
	}
	return next_motion;
}

void move() {
	int next_motion = offspring;

	// If My gradient is the maxest one, the only motion I need to execute
	// is to step forward.
	// Otherwise, I make my next motion decision based on a 6 * 6 matrix.
	if (flag_maxest == YES)
	{
		next_motion = FORWARD;
	}
	// case 1:
	else if (NEARER_INLINE)
	{
		set_color(RGB(1, 0, 0));
		next_motion = offspring;
		last_logic_1 = LOGIC_NEARER;
		last_logic_2 = LOGIC_INLINE;
	}
	// case 2:
	else if (NEARER_OUTLINE)
	{
		set_color(RGB(0, 1, 0));
		if (last_logic_2 == LOGIC_OUTLINE)
		{
			//set_color(RGB(0, 1, 0));
			next_motion = offspring;
		}
		else
		{
			//set_color(RGB(0, 1, 0));
			next_motion = opposite_move(offspring);
		}
		last_logic_1 = LOGIC_NEARER;
		last_logic_2 = LOGIC_OUTLINE;
	}
	// case 3:
	else if (EQUAL_INLINE)
	{
        set_color(RGB(0, 0, 1));
		if (last_logic_1 == LOGIC_NEARER)
		{
			//set_color(RGB(0, 1, 0));
			next_motion = opposite_move(offspring);
		}
		else
		{
			//set_color(RGB(1, 0, 0));
			next_motion = offspring;
		}
		last_logic_1 = LOGIC_EQUAL;
		last_logic_2 = LOGIC_INLINE;
	}
	// case 4:
	else if (EQUAL_OUTLINE)
	{
		set_color(RGB(1, 1, 0));
		if ((last_logic_1 != LOGIC_NEARER) && (last_logic_2 == LOGIC_OUTLINE))
		{
			//set_color(RGB(1, 0, 0));
			next_motion = offspring;
		}
		else
		{
			//set_color(RGB(0, 1, 0));
			next_motion = opposite_move(offspring);
		}
		last_logic_1 = LOGIC_EQUAL;
		last_logic_2 = LOGIC_OUTLINE;
	}
	// case 5:
	else if (FARER_INLINE)
	{
		set_color(RGB(1, 0, 1));
		if (last_logic_1 == LOGIC_FARER)
		{
			//set_color(RGB(1, 0, 0));
			next_motion = offspring;
		}
		else
		{
			//set_color(RGB(0, 1, 0));
			next_motion = opposite_move(offspring);
		}
		last_logic_1 = LOGIC_FARER;
		last_logic_2 = LOGIC_INLINE;
	}
	// case 6:
	else if (FARER_OUTLINE)
	{
		set_color(RGB(0, 1, 1));
		if ((last_logic_1 == LOGIC_FARER) && (last_logic_2 == LOGIC_OUTLINE))
		{
			//set_color(RGB(1, 0, 0));
			next_motion = offspring;
		}
		else
		{
			//set_color(RGB(0, 1, 0));
			next_motion = opposite_move(offspring);
		}
		last_logic_1 = LOGIC_FARER;
		last_logic_2 = LOGIC_OUTLINE;
	}

	// Update and carry out the decision maked above.
	offspring = next_motion;
	set_motion(offspring);
}


void loop() {
	//set_color(RGB(0, 0, 0));
	//set_led();
	check_own_gradient();
	// Move only when the sequence has already formed.
	// Move can only occured when the movitvator and motivated member
	// is stationary. This can assure the kilobot make the right
	// decision based on the measured changing distance.
	if ((formed_state == FORMED_OK) && (state_motivator == COMPLETED) && (state_motivated != MOVE))
	{
		if (flag_maxest == YES)
		{
			// When my motivator is closer enough can I move
			// to assure my motivator is not lost.
			if (distance_to_motivator <= DISTANCE_MOVE)
			{
				state_myself = MOVE;
			}
			else if (distance_to_motivator >= DISTANCE_STOP)
			{
				state_myself = COMPLETED;
			}
			else
			{
				state_myself = MOVE;
			}
		}
		// My gradient is not the maxest.
		else
		{
			// When my motivator is closer enough can I move
			// to assure my motivator is not lost.
			if (distance_to_motivator <= DISTANCE_MOVE)
			{
				state_myself = MOVE;
			}
			// The stop condition is independent.
			if (distance_to_motivated <= DISTANCE_COLLIDE)
			{
				state_myself = COMPLETED;
			}
		}
		// Move can only occure when I am in the MOVE state.
		if (state_myself == MOVE)
		{
			// Motion is detected every fixed time interval
			// TIME_LAST_MOTION_UPDATE.
			// This can assure the fixed distance kilobot move
			// in a fixed speed.
			// If distance is updated, then I can move according to
			// strategies.
			if ((update_distance_to_motivated == UPDATE) && (update_distance_to_motivator == UPDATE))
			{
				// Update.
				if (flag_minor == NO)
				{
					update_distance_to_motivator = UNUPDATE;
				}
				if (flag_maxest == NO)
				{
					update_distance_to_motivated = UNUPDATE;
				}

				distance_line = distance_to_motivated + distance_to_motivator;
				// Update.
				// My motivated stops for the first time.
				// So the distance_to_motivated becomes larger is due to
				// the motion of my motivated and not my fault.
				// Hence, the best values for distance_to_motivated_best and
				// distance_line_best needed to be initialized here.
				if (my_fault == NO)
				{
					//set_color(RGB(1, 0, 0));
					//delay(50);
					my_fault = YES;
					distance_to_motivated_best = distance_to_motivated;
					distance_line_best = distance_line;
				}

				//set_color(RGB(0, 1, 0));	
				move();
				last_motion_update = kilo_ticks;

				if (my_fault == YES)
				{
					// Update distance_to_motivated_best
					if (distance_to_motivated < distance_to_motivated_best)
					{
						distance_to_motivated_best = distance_to_motivated;
					}
					// Update distance_line_best
					if (distance_line < distance_line_best)
					{
						distance_line_best = distance_line;
					}
				}
			}
			// If the distance is long-term no updated, 
			// then I stop and wait.
			// This can assure my motion is determined by the strategy.
			else if (kilo_ticks > (last_motion_update + TIME_LAST_MOTION_UPDATE))
			{
				set_motion(STOP);
			}
		}
		// If I am not in the MOVE state, I know that I cannot move.
		else
		{
			//set_color(RGB(0, 0, 0));
			set_motion(STOP);
		}
    }
	// Stop when the sequence has not formed, or my motivator or motivated
	// is/are in the MOVE state.
	else
	{
		//set_color(RGB(0, 0, 0));
		state_myself = COMPLETED;
		set_motion(STOP);
	}
}


message_t *message_tx()
{
	message.data[0] = own_gradient;
	message.data[1] = formed_state;
	message.data[2] = state_myself;
	message.data[3] = distance_to_motivator;
/*
	// Debug.
	switch (state_myself)
    {
        case STOP:
            set_color(RGB(1, 0, 0));
            break;
        case MOVE:
            set_color(RGB(0, 1, 0));
            break;
        case COMPLETED:
            set_color(RGB(0, 0, 1));
            break;
        default:
             set_color(RGB(0, 1, 1));
             break;
    }
*/
	message.crc = message_crc(&message);
    	return &message;
}

void message_rx(message_t *m, distance_measurement_t *d)
{
	//set_color(RGB(0, 0, 0));
	//set_color(RGB(1, 0, 0));
    	received_gradient = m->data[0];
    	distance = estimate_distance(d);
	// In the valid distance.
	// This distance can be a constraint condition if needed.
	if (distance <= DISTANCE_GRADIENT)
	{
		// I have neighbours. Mark it.
		last_gradient_anchored = kilo_ticks;
		// The message was sent by my motivated.
		// I found someone's gradient maxer than mine in the world.
		// My formed state is determined by my motivated.
		if (received_gradient > own_gradient)
		{
			last_found_maxer = kilo_ticks;
			flag_maxest = NO;
			if (received_gradient == (own_gradient + 1))
			{
				formed_state = m->data[1];
				state_motivated =  m->data[2];
				update_state_motivated = UPDATE;
				if (state_motivated != MOVE)
				{
					// If this is the first time I detect the stationary
					// state of my motivated, I need to mark it because
					// the distance_to_motivated becomes larger is due to
					// the motion of my motivated in the last moment,
					// not my wrong decision making. 
					// This is important because I can only make
					// the right decision only when I know the truth.
					if ((++num_stop) == 1)
					{
						my_fault = NO;
					}
				}
				else
				{
					num_stop = 0;
				}
				distance_to_motivated = distance;
				update_distance_to_motivated = UPDATE;
			}
		}
		// That guy has the same gradient with mine.
		// There needs a comparison between us.
		else if ((received_gradient == own_gradient) && (received_gradient != GRADIENT_MAX))
		{
			distance_to_motivator_pair = m->data[3];
			if (distance_to_motivator_pair < distance_to_motivator)
			{
				last_found_minor = kilo_ticks;
				own_gradient = received_gradient + 1;
				state_motivator =  m->data[2];
				update_state_motivator = UPDATE;
				distance_to_motivator = distance;
				update_distance_to_motivator = UPDATE;
			}
		}
		// received_gradient < own_gradient
		// The message was sent by my motivator.
		else if (kilo_uid != SEED_ID)
		{
			// If four kilobots form a straght line, the distance between
			// two ends is 3 * 33mm = 100 mm.
			// The following code can assure that I am not affected by 
			// my motivator's motivator 
			// or even my motivator's motivator's motivator.
			// For example, own_gradient = 3;
			// my_motivator = 2;
			// my_motivator's motivator = 1;
			// my_motivator's motivator's motivator = 0;
			// 0, 1, 2, 3
			if (((own_gradient - received_gradient) == 2) || ((own_gradient - received_gradient) == 3))
			{
				// The message sender is closer, and meanwhile the last
				// time I found a minor is too long ago (TIME_CHECK_MINOR).
				// Thus I need  to find a new motivator.
				if ((distance < distance_to_motivator) && (kilo_ticks > (last_found_minor + TIME_CHECK_MINOR)))
				{
					last_found_minor = kilo_ticks;
					own_gradient = received_gradient + 1;
					state_motivator =  m->data[2];
					update_state_motivator = UPDATE;
					distance_to_motivator = distance;
					update_distance_to_motivator = UPDATE;
				}
				// Due to the message transfer delay, the following wrong
				// gradient formation may occure,  0, 2, 1.
				// The following code auotmatically modify this.
				// This code occures when 2 first find 1 and then find 0.
				else if (distance < distance_to_motivator_pair)
				{
					last_found_minor = kilo_ticks;
					own_gradient = received_gradient + 1;
					state_motivator =  m->data[2];
					update_state_motivator = UPDATE;
					distance_to_motivator = distance;
					update_distance_to_motivator = UPDATE;
				}
			}
			// This is the formal situation, my gradient is based on 
			// a miner.
			else
			{
				last_found_minor = kilo_ticks;
				own_gradient = received_gradient + 1;
				state_motivator =  m->data[2];
				update_state_motivator = UPDATE;
				distance_to_motivator = distance;
				update_distance_to_motivator = UPDATE;
			}
		}

		// I have neighbours whose gradient is minor than mine.
		// Meanwhile long time no find gradient maxer than mine.
		// I am the maxest one in my local world.
		if((kilo_uid != SEED_ID) && (kilo_ticks > (last_found_maxer + TIME_CHECK_MAXER)))
		{
			flag_maxest = YES;
			formed_state = FORMED_OK;
			state_motivated = COMPLETED;
			distance_to_motivated = DISTANCE_MAX;
			update_distance_to_motivated = UPDATE;
		}
		/*
		// I am the minor in the sequence.
		if (kilo_ticks > (last_found_minor + TIME_CHECK_MINOR))
		{
			flag_minor = YES;
			distance_to_motivator = DISTANCE_MOVE;
			update_distance_to_motivator = UPDATE;
			state_motivator = COMPLETED;
			update_state_motivator = UPDATE;
			set_color(RGB(1, 0, 1));
		}
		*/
	}
}



int main()
{
    kilo_init();
    kilo_message_rx = message_rx;
    kilo_message_tx = message_tx;
    kilo_start(setup, loop);

    return 0;
}
