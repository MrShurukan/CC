          if (elecCauldronTimeHyst == UNSET) setTimer(&elecCauldronTimeHyst, 62);
          else if (elecCauldronTimeHyst == TRIGGERED) switchElecCauldron(false);
        }
        else if (T[POD] > T[SETPOD] + hyst) {
          switchElecCauldron(false, LOCAL);
 
          if (heatMode == GASHEAT)
            switchGasCauldron(false);
          else if (heatMode == ELECTROHEAT)
            switchElecCauldron(false, LOCAL);
          else if (heatMode == AUTOHEAT)
            if (chosenCauldron == GAS) switchGasCauldron(false);
            else if (chosenCauldron == ELECTRO) switchElecCauldron(false, LOCAL);
        }
        else if (T[DOM] >= T[SETDOM] && activeHeat == REDHEAT && T[POD] < T[SETPOD] - hyst)    //Красный heat
          useHeat(heatMode);
        else if (T[DOM] < T[SETDOM] && T[POD] < T[SETPOD] - hyst) {
          if (activeHeat != GREENHEAT) {
            //            float diff = T[SETDOM] - T[DOM];
            //            if (diff >= 0.1 && diff <= 0.5) T[SETPOD] = 45.0;//heatTemp = 45;
            //            else if (diff >= 0.6 && diff <= 1.5) T[SETPOD] = 55.0;//heatTemp = 55;
            //            else if (diff >= 1.6) T[SETPOD] = 65.0;//heatTemp = 65;
 
            calcAutoTemp();
          }
          else T[SETPOD] = 65.0;
          switchElecCauldron(true);
        }
      }
    }
 
    //Теплый пол
    if (T[POD] >= 31 && activeHeat != GREENHEAT) digitalWrite(pinTPol, HIGH);
    else digitalWrite(pinTPol, LOW);
 
  }
  else {
    bool leds[] = {false, true, false};     //Зеленый
    useLeds(leds);
    if (csystemState != INACTIVE) {
      switchElecCauldron(false);
      switchGasCauldron(false, FULL);
    }
  }
}
