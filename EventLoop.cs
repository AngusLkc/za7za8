using System;
using System.Collections;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace Project
{
    public enum IOMode : byte
    {
        recv = 0x01,
        send = 0x02,
        error = 0x04
    }

    internal struct timeval
    {
        public int tv_sec;
        public int tv_usec;

        public timeval(long ms)
        {
            tv_sec = (int)(ms / 1000L);
            tv_usec = (int)(ms % 1000L) * 1000;
        }
    }

    public static class EventLoop
    {
        private static readonly Hashtable RecvList;
        private static readonly Hashtable SendList;
        private static readonly Hashtable ErrorList;

        private static readonly Dictionary<Socket, Action<Socket, IOMode>> EventCallback;
        private static readonly HashSet<Action> TimerSet;
        private static int _timeout_interval = 1000;

        public static int _last_time;
        public static bool _stopped;

        static EventLoop()
        {
            RecvList = new Hashtable();
            SendList = new Hashtable();
            ErrorList = new Hashtable();
            EventCallback = new Dictionary<Socket, Action<Socket, IOMode>>();
            TimerSet = new HashSet<Action>();
        }

        public static void Register(IOMode mode, Socket sock, Action<Socket, IOMode> func)
        {
            if ((mode & IOMode.recv) > 0)
            {
                if (!RecvList.Contains(sock.Handle))
                    RecvList.Add(sock.Handle, sock);
            }
            if ((mode & IOMode.send) > 0)
            {
                if (!SendList.Contains(sock.Handle))
                    SendList.Add(sock.Handle, sock);
            }
            if ((mode & IOMode.error) > 0)
            {
                if (!ErrorList.Contains(sock.Handle))
                    ErrorList.Add(sock.Handle, sock);
            }
            try
            {
                EventCallback.Add(sock, func);
            }
            catch
            {
                EventCallback[sock] = func;
            }
        }

        public static void Unregister(Socket sock)
        {
            if (RecvList.Contains(sock.Handle))
                RecvList.Remove(sock.Handle);
            if (SendList.Contains(sock.Handle))
                SendList.Remove(sock.Handle);
            if (ErrorList.Contains(sock.Handle))
                ErrorList.Remove(sock.Handle);
            if (EventCallback.ContainsKey(sock))
                EventCallback.Remove(sock);
        }

        public static void Modify(IOMode mode, Socket sock, Action<Socket, IOMode> func)
        {
            Unregister(sock);
            Register(mode, sock, func);
        }

        public static void AddTimerCB(Action func)
        {
            if (!TimerSet.Contains(func))
                TimerSet.Add(func);
        }

        public static void RemoveTimerCB(Action func)
        {
            if (TimerSet.Contains(func))
                TimerSet.Remove(func);
        }

        public static void Run(long interval)
        {
            _stopped = false;
            _last_time = Environment.TickCount;
            timeval tv;
            while (true)
            {
                if (_stopped)
                {
                    RecvList.Clear();
                    SendList.Clear();
                    ErrorList.Clear();
                    EventCallback.Clear();
                    TimerSet.Clear();
                    _last_time = Environment.TickCount;
                    return;
                }
                if (RecvList.Count == 0 && SendList.Count == 0 && ErrorList.Count == 0 && TimerSet.Count == 0)
                {
                    _stopped = true;
                    continue;
                }
                IntPtr[] rlist = SocketSetToIntPtrArray(RecvList);
                IntPtr[] slist = SocketSetToIntPtrArray(SendList);
                IntPtr[] elist = SocketSetToIntPtrArray(ErrorList);
                int res, i;
                tv = new timeval(interval);
                if (interval > 0)
                    res = select(0, rlist, slist, elist, ref tv);
                else
                    res = select(0, rlist, slist, elist, IntPtr.Zero);
                if (_stopped)
                    continue;
                lock (OpenTelnet.synclock)
                {
                    if (res > 0)
                    {   //rlist、wlist、xlist第一个元素表示文件描述符数量
                        if (rlist != null && (int)rlist[0] > 0)
                        {
                            for (i = 1; i <= (int)rlist[0]; i++)
                            {
                                EventCallback[(Socket)RecvList[rlist[i]]]((Socket)RecvList[rlist[i]], IOMode.recv);
                            }
                        }
                        if (slist != null && (int)slist[0] > 0)
                        {
                            for (i = 1; i <= (int)slist[0]; i++)
                            {
                                EventCallback[(Socket)SendList[slist[i]]]((Socket)SendList[slist[i]], IOMode.send);
                            }
                        }
                        if (elist != null && (int)elist[0] > 0)
                        {
                            for (i = 1; i <= (int)elist[0]; i++)
                            {
                                EventCallback[(Socket)ErrorList[elist[i]]]((Socket)ErrorList[elist[i]], IOMode.error);
                            }
                        }
                    }
                    else if (res == 0)
                    {
                        //Logging.Error("SELECT调用超时");
                    }
                    else
                    {
                        //Logging.Error($"SELECT调用错误:{GetErrorMsg(GetLastError())}");
                    }
                    int current_timestamp = Environment.TickCount;
                    if (current_timestamp - _last_time >= _timeout_interval)
                    {
                        Action[] clone = new Action[TimerSet.Count];
                        TimerSet.CopyTo(clone);
                        foreach (Action func in clone)
                            func();
                        _last_time = current_timestamp;
                    }
                }
            }
        }

        internal static IntPtr[] SocketSetToIntPtrArray(Hashtable SocketList)
        {
            if (SocketList == null || SocketList.Count < 1)
                return null;
            IntPtr[] array = new IntPtr[SocketList.Count + 1];
            array[0] = (IntPtr)SocketList.Count;
            int i = 0;
            foreach (IntPtr ptr in SocketList.Keys)
            {
                array[i + 1] = ptr;
                i += 1;
            }
            return array;
        }

        internal const int FORMAT_MESSAGE_FROM_SYSTEM = 0x00001000;
        internal const int FORMAT_MESSAGE_IGNORE_INSERTS = 0x00000200;
        internal const int FORMAT_MESSAGE_ALLOCATE_BUFFER = 0x00000100;

        internal static string GetErrorMsg(int code)
        {
            int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, IntPtr.Zero, code, 0, out string describe, 255, IntPtr.Zero);
            if (len > 0)
                return describe.Remove(len);
            return null;
        }

        [DllImport("ws2_32.dll", SetLastError = true)]
        internal static extern int select([In] int ignoredParam, [In][Out] IntPtr[] readfds, [In][Out] IntPtr[] writefds, [In][Out] IntPtr[] exceptfds, [In] ref timeval timeout);

        [DllImport("ws2_32.dll", SetLastError = true)]
        internal static extern int select([In] int ignoredParam, [In][Out] IntPtr[] readfds, [In][Out] IntPtr[] writefds, [In][Out] IntPtr[] exceptfds, [In] IntPtr nullTimeout);

        [DllImport("kernel32.dll")]
        internal static extern int GetLastError();

        [DllImport("kernel32.dll")]
        internal static extern int FormatMessage(int flag, IntPtr source, int msgid, int langid, out string buf, int size, IntPtr args);
    }
}
