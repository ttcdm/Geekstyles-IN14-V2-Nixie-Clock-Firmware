#ifndef _MACRO_H_
#define	_MACRO_H_

#ifdef	__cplusplus
extern "C" {
#endif


#define bin(a) ((( (a/10000000*128) + \
(((a/1000000)&1)*64) + \
(((a/100000)&1)*32) + \
(((a/10000)&1)*16) + \
(((a/1000)&1)*8) + \
(((a/100)&1)*4) + \
(((a/10)&1)*2) + \
(a&1)) * (a/10000000)) + \
(( ((a/262144)*64) + \
(((a/32768)&1)*32) + \
(((a/4096)&1)*16) + \
(((a/512)&1)*8) + \
(((a/64)&1)*4) + \
(((a/8)&1)*2) + \
(a&1)) * (1-(a/10000000))))

#define _BV(bitno) (1 << (bitno))
#define bittoggle(var, bitno)    ((var) ^= _BV(bitno))
#define bitset(var, bitno)    ((var) |= _BV(bitno))
#define bitclr(var, bitno)    ((var) &= ~_BV(bitno))
#define bitchk(var, bitno)    ((var) & _BV(bitno))
#define bitmaskset(var, bitmask)	(var |= bitmask)
#define bitmaskclr(var, bitmask)	(var &= ~bitmask)
#define bitmaskchk(var, bitmask)	((var & bitmask)==bitmask)

#define	hibyte(v1)		((uint8_t)((v1)>>8))
#define	lobyte(v1)		((uint8_t)((v1)&0xff))

/// scale a number from one range to another (like map() but with 0 offsets)
#define scale(x,inMax,outMax)      (((int32_t)x*(int32_t)outMax)/inMax)
/// re-map a number from one range to another
#define map(x,inMin,inMax,outMin,outMax)    ((int32_t)(x-inMin)*(int32_t)(outMax-outMin)/(inMax-inMin)+outMin)

#ifdef	__cplusplus
}
#endif

#endif	/* _MACRO_H_ */