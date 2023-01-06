/*******************************************************************************
  Copyright(c) 2017 Simon Holmbo. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <memory>
#include <map>

#include "indicom.h"
#include "indi_shelyakeshel_spectrograph.h"
#include "config.h"

const char *SPECTROGRAPH_SETTINGS_TAB = "Spectrograph Settings";
const char *CALIBRATION_UNIT_TAB      = "Calibration Unit";

static std::map<ISState, char> COMMANDS = {
    { ISS_ON, 0x53 },
    { ISS_OFF, 0x43 }
};

static std::map<std::string, char> PARAMETERS = {
    { "MIRROR", 0x31 },
    { "LED", 0x32 },
    { "THAR", 0x33 },
    { "TUNGSTEN", 0x34 }
};

std::unique_ptr<ShelyakEshel>
    shelyakEshel(new ShelyakEshel()); // create std:unique_ptr (smart pointer) to  our spectrograph object

ShelyakEshel::ShelyakEshel()
{
    PortFD = -1;

    setVersion(SHELYAK_ESHEL_VERSION_MAJOR, SHELYAK_ESHEL_VERSION_MINOR);
}

ShelyakEshel::~ShelyakEshel() {}

/* Returns the name of the device. */
const char *ShelyakEshel::getDefaultName()
{
    return (char *)"Shelyak eShel";
}

/* Initialize and setup all properties on startup. */
bool ShelyakEshel::initProperties()
{
    INDI::DefaultDevice::initProperties();

    //--------------------------------------------------------------------------------
    // Calibration Unit
    //--------------------------------------------------------------------------------

    // setup the mirror switch
    IUFillSwitch(&MirrorS[0], "ACTIVATED", "Activated", ISS_OFF);
    IUFillSwitch(&MirrorS[1], "DEACTIVATED", "Deactivated", ISS_ON);
    IUFillSwitchVector(&MirrorSP, MirrorS, 2, getDeviceName(), "FLIP_MIRROR", "Flip mirror", CALIBRATION_UNIT_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // setup the lamp switches
    IUFillSwitch(&LampS[0], "LED", "LED", ISS_OFF);
    IUFillSwitch(&LampS[1], "THAR", "ThAr", ISS_OFF);
    IUFillSwitch(&LampS[2], "TUNGSTEN", "Tungsten", ISS_OFF);
    IUFillSwitchVector(&LampSP, LampS, 3, getDeviceName(), "CALIB_LAMPS", "Calibration lamps", CALIBRATION_UNIT_TAB,
                       IP_RW, ISR_NOFMANY, 0, IPS_IDLE);

    //--------------------------------------------------------------------------------
    // Options
    //--------------------------------------------------------------------------------

    // setup the text input for the serial port
    IUFillText(&PortT[0], "PORT", "Port", "/dev/ttyUSB0");
    IUFillTextVector(&PortTP, PortT, 1, getDeviceName(), "DEVICE_PORT", "Ports", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    //--------------------------------------------------------------------------------
    // Spectrograph Settings
    //--------------------------------------------------------------------------------

    IUFillNumber(&SettingsN[0], "GRATING", "Grating [lines/mm]", "%.2f", 0, 1000, 0, 79);
    IUFillNumber(&SettingsN[1], "INCIDENCE_ANGLE_ALPHA", "Incidence angle alpha [degrees]", "%.2f", 0, 90, 0, 62.2);
    IUFillNumber(&SettingsN[2], "DIFFRACTION_ANGLE_BETA", "Diffraction angle beta [degrees]", "%.2f", 0, 90, 0, 0);
    IUFillNumber(&SettingsN[3], "SHIFT_ANGLE_GAMMA", "Shift angle gamma [degrees]", "%.2f", 0, 90, 0, 5.75);
    IUFillNumber(&SettingsN[4], "OBJ_FOCAL", "Obj Focal [mm]", "%.0f", 1, 700, 0, 85);
    IUFillNumberVector(&SettingsNP, SettingsN, 5, getDeviceName(), "SPECTROGRAPH_SETTINGS", "Spectrograph settings",
                       SPECTROGRAPH_SETTINGS_TAB, IP_RW, 60, IPS_IDLE);

    setDriverInterface(SPECTROGRAPH_INTERFACE);

    return true;
}

void ShelyakEshel::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
    defineProperty(&PortTP);
    defineProperty(&SettingsNP);
    loadConfig(true, PortTP.name);
}

bool ShelyakEshel::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    if (isConnected())
    {
        // create properties if we are connected
        defineProperty(&MirrorSP);
        defineProperty(&LampSP);
    }
    else
    {
        // delete properties if we aren't connected
        deleteProperty(MirrorSP.name);
        deleteProperty(LampSP.name);
    }
    return true;
}

bool ShelyakEshel::Connect()
{
    int rc;
    char errMsg[MAXRBUF];
    if ((rc = tty_connect(PortT[0].text, 2400, 8, 0, 1, &PortFD)) != TTY_OK)
    {
        tty_error_msg(rc, errMsg, MAXRBUF);
        LOGF_ERROR("Failed to connect to port %s. Error: %s", PortT[0].text, errMsg);
        return false;
    }
    LOGF_INFO("%s is online.", getDeviceName());
    return true;
}

bool ShelyakEshel::Disconnect()
{
    tty_disconnect(PortFD);
    LOGF_INFO("%s is offline.", getDeviceName());
    return true;
}

/* Handle a request to change a switch. */
bool ShelyakEshel::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (!strcmp(dev, getDeviceName())) // check if the message is for our device
    {
        if (!strcmp(LampSP.name, name)) // check if its lamp request
        {
            LampSP.s = IPS_OK; // set state to ok (change later if something goes wrong)
            for (int i = 0; i < n; i++)
            {
                ISwitch *s = IUFindSwitch(&LampSP, names[i]);
                if (states[i] != s->s)
                { // check if state has changed
                    bool rc = calibrationUnitCommand(COMMANDS[states[i]], PARAMETERS[names[i]]);
                    if (!rc)
                        LampSP.s = IPS_ALERT;
                }
            }
            IUUpdateSwitch(&LampSP, states, names, n); // update lamps
            IDSetSwitch(&LampSP, nullptr);             // tell clients to update
            return true;
        }
        else

            if (!strcmp(MirrorSP.name, name)) // check if its a mirror request
        {
            ISState s = MirrorS[0].s;                    // save current mirror state
            IUUpdateSwitch(&MirrorSP, states, names, n); // update mirror
            MirrorSP.s = IPS_OK;                         // set state to ok (change later if something goes wrong)
            if (s != MirrorS[0].s)
            { // if mirror state has changed send command
                bool rc = calibrationUnitCommand(COMMANDS[MirrorS[0].s], PARAMETERS["MIRROR"]);
                if (!rc)
                    MirrorSP.s = IPS_ALERT;
            }
            IDSetSwitch(&MirrorSP, nullptr); // tell clients to update
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n); // send it to the parent classes
}

/* Handle a request to change text. */
bool ShelyakEshel::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (!strcmp(dev, getDeviceName())) // check if the message is for our device
    {
        if (!strcmp(PortTP.name, name)) //check if is a port change request
        {
            IUUpdateText(&PortTP, texts, names, n); // update port
            PortTP.s = IPS_OK;                      // set state to ok
            IDSetText(&PortTP, nullptr);            // tell clients to update the port
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

/* Construct a command and send it to the spectrograph. It doesn't return
 * anything so we have to sleep until we know it has flipped the switch.
 */
bool ShelyakEshel::calibrationUnitCommand(char command, char parameter)
{
    int rc, nbytes_written;
    char c[5] = { 0x0D, 0x01, command,
                  parameter }; // the first 2 bytes are constant, the next 2 are command and parameter
    c[4]      = 0x100 - (c[0] + c[1] + c[2] + c[3]) % 0x100; // last byte is is related to the sum of the four first
    if ((rc = tty_write(PortFD, c, 5, &nbytes_written)) != TTY_OK) // send the bytes to the spectrograph
    {
        char errmsg[MAXRBUF];
        tty_error_msg(rc, errmsg, MAXRBUF);
        LOGF_ERROR("error: %s.", errmsg);
        return false;
    }
    sleep(1); // wait for the calibration unit to actually flip the switch
    return true;
}
