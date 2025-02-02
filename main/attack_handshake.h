/**
 * @file attack_handshake.h
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Provides interface to control attacks on WPA handshake
 */
#ifndef ATTACK_HANDSHAKE_H
#define ATTACK_HANDSHAKE_H

#include "attack.h"

/**
 * @brief Available methods that can be chosen for the attack.
 * 
 */
/**
 * @brief Starts handshake attack with given attack config.
 * 
 * To stop handshake attack, call attack_handshake_stop().
 * 
 * @param attack_config attack config with valid ap_record and attack method chosen 
 */
void attack_handshake_start(attack_config_t *attack_config);

/**
 * @brief Stops handshake attack.
 * 
 * This function stops everything that attack_handshake_start() started and resets all values to default state.
 */
void attack_handshake_stop();

#endif