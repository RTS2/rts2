import re

__fits_array = re.compile('\[(\d+):(\d+),(\d+):(\d+)\]')

def parse_detsec(value):
    """
    Parse FITS array header value ([x1:x2,y1:y2])

    Parameters
    ----------
    value: FITS header string in rectange format

    Returns
    -------
    4 member array or none if 
    """
    try:
        return list(map(float, __fits_array.match(value).groups()))
    except AttributeError(er):
        raise ValueError('wrong value format: ' + value)
