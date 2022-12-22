/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
*
* \class AscentData 
* \brief Ascent platform constants 
* 
*/ 


#ifndef ASCENTDATA_INCLUDE_H__ 
#define ASCENTDATA_INCLUDE_H__ 

#include "PlatformData.h"

class AscentData : public PlatformData
{ 
    public: 
        AscentData();
        virtual ~AscentData(); 

    private:
        //disabling the copy ctor and assignment operator
        //generated by the compiler - don't want them
        //Effective C++ Item 6
        AscentData(const AscentData&);
        AscentData& operator=(AscentData&);

}; 

#endif
