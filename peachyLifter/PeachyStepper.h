// This is Peachys Stepper motor driver
// It uses a half-stepping for 4 coils
#ifndef _PEACHYSTEPPER_H_
#define _PEACHYSTEPPER_H_

//The digital pins used for the stepper driver
#define STEPPER_PIN0 8
#define STEPPER_PIN1 9
#define STEPPER_PIN2 10
#define STEPPER_PIN3 11

// How many times step gets called (minus one) between actual motor steps
// These steps include the 0th step,
// ie: 3 means 0->1->2->3 (4 calls)
#define STEPPER_U_STEPS 3 //Must be greater than 0

//These shouldn't need to be changed
#define STEPPER_STABLE 2
#define STEPPER_UP 1
#define STEPPER_DOWN 0

#define MASK_TOPBIT 0b10000000
#define MASK_BOTBIT 0b00000001

#define MASK_PIN0 0b00000010
#define MASK_PIN1 0b00001000
#define MASK_PIN2 0b00100000
#define MASK_PIN3 0b10000000

#include <stdint.h>
class PeachyStepper
{
	public:
		PeachyStepper(uint8_t hold_torque = STEPPER_U_STEPS){
			m_current_position=0;
			m_commanded_position=0;
			m_direction = STEPPER_STABLE;
			m_hold_torque = hold_torque; 
			m_step_state = 7;//[00000111]
			m_microstep_counter = 0;
		}

		void moveTo(uint32_t position){
			m_commanded_position=position;
		}

		void move(uint8_t direction,uint32_t steps){
			//direction 0 or 1 depending on the wiring
			if (direction == STEPPER_DOWN)
				m_commanded_position-=steps;
			else if (direction == STEPPER_UP)
				m_commanded_position+=steps;
		}

		void move (uint8_t direction){
			//direction 0 or 1 depending on the wiring
			if (direction == STEPPER_DOWN)
				m_commanded_position--;
			else if (direction == STEPPER_UP)
				m_commanded_position++;
		}

		void step(){
			if (m_current_position == m_commanded_position){
				m_direction=STEPPER_STABLE;
			}
			else {
				if (m_current_position < m_commanded_position){
					m_direction = STEPPER_UP;
					m_current_position++;
				}
				else{ // <
					m_direction = STEPPER_DOWN;
					m_current_position--;
				}
				shift_step();
				assign_bits();
			}
		}

		//Function called by the timer interrupt 
		void micro_step(){
			m_microstep_counter=(m_microstep_counter%STEPPER_U_STEPS)+1; //0->STEPPER_U_STEPS inclusive
			
			if (m_current_position == m_commanded_position){
				//Switch to holding torque mode
				m_direction=STEPPER_STABLE;

				//ie: hold_torque==2, counter==3, microsteps==0->3 -> pins off for that one cycle 
				if (m_microstep_counter>m_hold_torque){ 
					pins_off();
				}
				//ie: otherwise keep those pins on for the other 3 cycles (0,1,2)
				else{ 
					pins_on();
				}
				
			}
			else if (m_microstep_counter==0){
				if (m_current_position < m_commanded_position){
					m_direction = STEPPER_UP;
					m_current_position++;
				}
				else{ // <
					m_direction = STEPPER_DOWN;
					m_current_position--;
				}

				shift_step();
				assign_bits();
			}
		}

		void stop(){
			m_commanded_position=m_current_position;
		}

		void zeroPosition(){
			m_current_position = 0;
			m_commanded_position = 0;
		}

		int32_t getPosition(){
			return m_current_position;
		}

		int32_t getCommandedPosition(){
			return m_commanded_position;
		}

		uint8_t getState(){
			return m_step_state;
		}

		uint8_t getDirection(){
			return m_direction;
		}

	private:
		uint8_t m_direction;
		uint8_t m_hold_torque;
		uint8_t m_step_state;
		uint8_t m_microstep_counter;
		int32_t m_commanded_position;
		int32_t m_current_position;

		void shift_step(){
			uint8_t carry_bit;

			if (m_direction == STEPPER_UP){
				carry_bit=m_step_state & MASK_TOPBIT;
				m_step_state=m_step_state<<1; //Shift up
				if (carry_bit==0x80)
					m_step_state+=0x01;//Bring in the carry
			}
			else if(m_direction == STEPPER_DOWN){
				carry_bit=m_step_state & MASK_BOTBIT;
				m_step_state=m_step_state>>1; //Shift down
				if (carry_bit==0x01)
					m_step_state+=0x80;//Add in the carry at the top
			}
		}

		void pins_off(){
			digitalWrite(STEPPER_PIN0,0);
			digitalWrite(STEPPER_PIN1,0);
			digitalWrite(STEPPER_PIN2,0);
			digitalWrite(STEPPER_PIN3,0);
		}

		void pins_on(){
			assign_bits();
		}

		void assign_bits()
		{
			// 4 pin servos "on" steps:
			// [pin0 pin1 pin2 pin3]
			// [1 0 0 0] 0 
			// [1 1 0 0] 1 
			// [0 1 0 0] 2 
			// [0 1 1 0] 3 
			// [0 0 1 0] 4 
			// [0 0 1 1] 5 
			// [0 0 0 1] 6 
			// [1 0 0 1] 7 
			uint8_t tmp_bit;

			tmp_bit = (m_step_state & MASK_PIN0);
			digitalWrite(STEPPER_PIN0,tmp_bit);

			tmp_bit = (m_step_state & MASK_PIN1);
			digitalWrite(STEPPER_PIN1,tmp_bit);

			tmp_bit = (m_step_state & MASK_PIN2);
			digitalWrite(STEPPER_PIN2,tmp_bit);

			tmp_bit = (m_step_state & MASK_PIN3);
			digitalWrite(STEPPER_PIN3,tmp_bit);
		}
};
#endif