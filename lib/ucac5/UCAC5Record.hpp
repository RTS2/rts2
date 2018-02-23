#ifndef __UCAC5RECORD__
#define __UCAC5RECORD__

#include "erfa.h"

#include <stdint.h>

// This structure describes one record of data read from the UCAC4 catalog.
// This is 40 bytes long. The references to notes refer to notes listed in
// The readmeU5.*.txt file.
#pragma pack(1)
struct ucac5 { 
  uint64_t srcid;     // Gaia source ID
  int32_t  rag;       // mas     Gaia RA  at epoch 2015.0  (from DR1, rounded to mas)
  int32_t  dcg;       // mas     Gaia Dec at epoch 2015.0 
  uint16_t erg;       // 0.1 mas Gaia DR1 position error RA  at epoch 2015.0
  uint16_t edg;       // 0.1 mas Gaia DR1 position error Dec at epoch 2015.0
  uint8_t  flg;       // 1 = TGAS,  2 = other UCAC-Gaia star, 3 = other NOMAD 
  uint8_t  nu;        // number of images used for UCAC mean position
  uint16_t epu;       // myr     mean UCAC epoch (1/1000 yr after 1997.0)
  int32_t  ira;       // mas     mean UCAC RA  at epu epoch on Gaia reference frame
  int32_t  idc;       // mas     mean UCAC Dec at epu epoch on Gaia reference frame
  int16_t  pmur;      // 0.1 mas/yr  proper motion RA*cosDec (UCAC-Gaia)
  int16_t  pmud;      // 0.1 mas/yr  proper motion Dec       (UCAC-Gaia)
  uint16_t pmer;      // 0.1 mas/yr  formal error of UCAC-Gaia proper motion RA*cosDec
  uint16_t pmed;      // 0.1 mas/yr  formal error of UCAC-Gaia proper motion Dec 
  int16_t  gmag;      // mmag    Gaia DR1 G magnitude      (1/1000 mag)
  int16_t  umag;      // mmag    mean UCAC model magnitude (1/1000 mag)
  int16_t  rmag;      // mmag    NOMAD photographic R mag  {1/1000 mag)
  int16_t  jmag;      // mmag    2MASS J magnitude         (1/1000 mag)
  int16_t  hmag;      // mmag    2MASS H magnitude         (1/1000 mag)
  int16_t  kmag;      // mmag    2MASS K magnitude         (1/1000 mag)
};
#pragma pack()

/**
 * Represents UCAC5 record.
 */
class UCAC5Record
{
	public:
		/**
		 * Creates UCAC5 record from data read from index file.
		 *
		 * @param _data pointer to UCAC5 data
		 */
		UCAC5Record (struct ucac5 *_data);

		/**
		 * Returns XYZ unit vector of RA DEC coordinates.
		 */
		void getXYZ (double c[3]);
	
	private:
		struct ucac5 data;

};

#endif // !__UCAC5RECORD__
