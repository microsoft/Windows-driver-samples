// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
//
// Abstract:
//
//     This file contains helper methods that provide a friendly way to access win32 window functions.

using System;
using System.Runtime.InteropServices;

namespace Microsoft.Samples.Printing.PrinterExtension.Helpers
{
    class WindowHelper
    {
        /// <summary>
        /// P/Invoke signature for Win32 function "SetForegroundWindow".
        /// </summary>
        /// <param name="hwnd">Handle to window</param>
        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("User32", CharSet = CharSet.Auto, SetLastError = true, ExactSpelling = true)]
        public static extern bool SetForegroundWindow(IntPtr hwnd);

        /// <summary>
        /// Wrapper for Win32 function "FlashWindowEx"
        /// </summary>
        /// <param name="hWnd">Handle of the window to flash</param>
        public static bool FlashWindow(IntPtr hWnd)
        {
            FLASHWINFO fInfo = new FLASHWINFO();

            fInfo.cbSize = Convert.ToUInt32(Marshal.SizeOf(fInfo));
            fInfo.hwnd = hWnd;                              // Handle to window
            fInfo.uCount = UInt32.MaxValue;                 // Number of times to flash
            fInfo.dwTimeout = 0;                            // Use default cursor blink rate
            // Flash both window caption and taskbar button, until the window is brought to the foreground.
            fInfo.dwFlags = (uint)(FLASHW.ALL | FLASHW.TIMERNOFG);

            return FlashWindowEx(ref fInfo);
        }

        #region private members

        /// <summary>
        /// P/Invoke signature for Win32 function "FlashWindowEx".
        /// </summary>
        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("User32", CharSet = CharSet.Auto, SetLastError = false, ExactSpelling = true)]
        private static extern bool FlashWindowEx(ref FLASHWINFO pwfi);

        [StructLayout(LayoutKind.Sequential)]
        private struct FLASHWINFO
        {
            public UInt32 cbSize;
            public IntPtr hwnd;
            public UInt32 dwFlags;
            public UInt32 uCount;
            public UInt32 dwTimeout;
        }

        /// <summary>
        /// Represents the FLASH_Xxx flags
        /// </summary>
        [Flags]
        private enum FLASHW : uint
        {
            /// <summary>
            /// Flash both the window caption and taskbar button
            /// </summary>
            ALL = 3,
            /// <summary>
            /// Flash continuously until the window comes to the foreground.
            /// </summary>
            TIMERNOFG = 12
        }

        #endregion
    }
}
