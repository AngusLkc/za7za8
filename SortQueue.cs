using System;

namespace xxxxxxxxxxx
{
    public class SortQueue
    {
		private uint size;
        private uint index;
        private byte[][] buffer;

        private static uint roundup_pow_of_two(uint x)
        {
            uint i, b = 1;
            for (i = 0; i < 32; i++)
            {
                b <<= 1;
                if (x <= b)
                    break;
            }
            return b;
        }

        public SortQueue(uint capacity)
        {
            index = 0;
            size = capacity;
            if ((size & (size - 1)) > 0)
                size = roundup_pow_of_two(size);
            buffer = new byte[size][];
        }

        public bool Put(byte[] data, uint seq)
        {
            if (seq - index >= size)
                throw new Exception($"seq value illegal!");
            uint idx = seq & (size - 1);
            if (buffer[(int)idx] != null)
                return false;
            buffer[(int)idx] = data;
            return true;
        }

        public byte[][] Get(ref uint count)
        {
            uint len, once;
            for (len = 0; len < size; len++)
                if (buffer[(int)((index + len) & (size - 1))] == null)
                    break;
            byte[][] data = new byte[len][];
            once = Math.Min(len, size - (index & (size - 1)));
            Array.Copy(buffer, (int)(index & (size - 1)), data, 0, (int)once);
            Array.Clear(buffer, (int)(index & (size - 1)), (int)once);
            if (len - once > 0)
            {
                Array.Copy(buffer, 0, data, once, len - once);
                Array.Clear(buffer, 0, (int)(len - once));
            }
            count = len;
            index += len;
            return data;
        }
    }
}
