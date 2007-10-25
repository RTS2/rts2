
#define APISA_READ_USHORT _IOR('a', 0x01, unsigned int)
#define APISA_READ_LINE   _IOR('a', 0x02, unsigned int)
#define APISA_WRITE_USHORT  _IOW('a', 0x03, unsigned int)

#define APPPI_READ_USHORT _IOR('a', 0x01, unsigned int)
#define APPPI_READ_LINE   _IOR('a', 0x02, unsigned int)
#define APPPI_WRITE_USHORT  _IOW('a', 0x03, unsigned int)

#define APPCI_READ_USHORT _IOR('a', 0x01, unsigned int)
#define APPCI_READ_LINE   _IOR('a', 0x02, unsigned int)
#define APPCI_WRITE_USHORT  _IOW('a', 0x03, unsigned int)

#define appci_major_number 60
#define apppi_major_number 61
#define apisa_major_number 62

struct apIOparam				 // IOCTL data
{
	unsigned int reg;
	unsigned long param1, param2;
};

#define APOGEE_PCI_DEVICE "/dev/appci"
#define APOGEE_PPI_DEVICE "/dev/apppi"
#define APOGEE_ISA_DEVICE "/dev/apisa"
