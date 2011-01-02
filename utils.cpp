int writeHex(int c)
{
    if(c<10) return '0'+c;
    else return 'A'+(c-10);
}

int readHex(int c)
{
	switch(c) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return c - '0';
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			return c - 'a' + 10;
			break;
	}
}

int readBCD(int BCD)
{
	return 10*(BCD >> 4) + (BCD & 0x15);
}

