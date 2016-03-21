#include "PeachyPrintState.h"

extern PeachyFlagger g_Flagger;
extern PeachyStepper g_Stepper;

PeachyPrintState::PeachyPrintState(){
	m_layerHeight_mm = MICROMETERS_PER_LAYER*1e3; //for now
	m_layerHeight_steps = (uint16_t)((m_layerHeight_mm*1000)/MICROMETERS_PER_STEP);
	m_resin_height_steps = 0;
	m_printState = PRINT_STATE_PRINTING;
}

void PeachyPrintState::initializeFlags(){
	m_flagger_id_state = g_Flagger.registerFlag(0); //disabled for now
	m_flagger_id_picture = g_Flagger.registerFlag(0); //disabled for now
}

void PeachyPrintState::initializeStateDistanceTime(uint8_t state, float delay, float height_from_resin, uint8_t photoBeforeDelay, uint8_t photoAfterDelay, uint8_t externalTrigger){
			uint16_t tick_delay = delay/TICK_TIME;
			int32_t step_height = (height_from_resin/MILLIMETERS_PER_STEP)+m_resin_height_steps;
			initializeState(state, tick_delay, step_height, photoBeforeDelay, photoAfterDelay);
}

void PeachyPrintState::initializeState(uint8_t state, uint16_t delay, int32_t height ,uint8_t photoDuringDelay, uint8_t photoBeforeDelay, uint8_t photoAfterDelay, uint8_t externalTrigger){
	m_printStates[state].delay_ticks=delay;
	m_printStates[state].absoluteHeight_steps=height;
	m_printStates[state].photoBeforeDelay=photoBeforeDelay;
	m_printStates[state].photoAfterDelay=photoAfterDelay;
	m_printStates[state].photoDuringDelay=photoDuringDelay;
	m_printStates[state].externalTrigger=externalTrigger;
}

void PeachyPrintState::takePicture(){
	//quick snap
	g_Flagger.updateTrigCount(m_flagger_id_picture, PICTURE_QUICK_TICKS); //enable it and clear it 
	m_picture_pin_state=1;
	digitalWrite(CAMERA_PIN,1);
}

void PeachyPrintState::takeDuringPicture(){
	//Keep it on for a longer delay
	g_Flagger.updateTrigCount(m_flagger_id_picture, PICTURE_LONG_TICKS); //enable it and clear it 
	m_picture_pin_state=1;
	digitalWrite(CAMERA_PIN,1);

}

void PeachyPrintState::handlePrintStates(){
	pictureHandler();
	handleFinishedPrintState();
	handleStartPrintState();
}

void PeachyPrintState::pictureHandler(){
	//Figure out when to turn it off
	if (g_Flagger.getFlag(m_flagger_id_picture)){
		digitalWrite(CAMERA_PIN,0);
		m_picture_pin_state=0;
		g_Flagger.disable(m_flagger_id_picture);
		g_Flagger.clearFlag(m_flagger_id_picture);
	}
}

void PeachyPrintState::handleFinishedPrintState(){
	static uint8_t taken_after_picture=0;

	//check for: Ready for next state, AND we aren't currently taking a picture
	if (m_picture_pin_state == 0){
		if (g_Flagger.getFlag(m_flagger_id_state)){
			if ((m_printStates[m_printState].photoAfterDelay) & (taken_after_picture == 0)){
				taken_after_picture=1;
				takePicture(); //sets picture_pin to 1 until picture taking is done
			}
			else{
				taken_after_picture=0;
				m_finished_state=true;//Done!
				g_Flagger.clearFlag(m_flagger_id_state);
			}
		}
	}
}

void PeachyPrintState::handleStartPrintState(){
	static uint8_t taken_during_picture=0;
	static uint8_t taken_before_picture=0;
	static uint8_t update_trig_count=0;
	
	//Check that we still aren't taking a picture
	if (m_picture_pin_state == 0){

		//Finish the current State
		if (m_finished_state) {
			if (m_printStates[m_printState].externalTrigger == true ){
				if (m_external_triggered){
					m_finished_state=false;
					update_trig_count=1;
					g_Stepper.moveTo(m_printStates[m_printState].absoluteHeight_steps);
				}
			}
			else{
				m_finished_state=false;
				update_trig_count=1;
				g_Stepper.moveTo(m_printStates[m_printState].absoluteHeight_steps);
			}
		}

		//Else kick off the next state (when done moving)
		else if (g_Stepper.getDirection() == STEPPER_STABLE)  {
			if (update_trig_count == 1){
				m_external_triggered=false; //Zero the external trigger every time we update the trig count
				update_trig_count = 0;
				g_Flagger.updateTrigCount(m_flagger_id_state, m_printStates[m_printState].delay_ticks);
			}
			else if ((m_printStates[m_printState].photoBeforeDelay) & (taken_before_picture == 0)){
				taken_before_picture=1;
				takePicture();
			}
			else if ((m_printStates[m_printState].photoDuringDelay) & (taken_during_picture == 0)){
				taken_during_picture=1;
				takeDuringPicture();
			}
		}
	}
}