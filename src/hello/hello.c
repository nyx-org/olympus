void _start()
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(0), "D"("hello"));

    while (1)
    {
    }
}
