﻿using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SemtechLib.Devices.SX1231.Events
{
    [Serializable, ComVisible(true)]
    public delegate void ClockOutEventHandler(object sender, ClockOutEventArg e);
}

