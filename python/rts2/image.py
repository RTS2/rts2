def parse_detsec(value):
    """
    Parse FITS array header value ([x1:x2,y1:y2]). Throws exception if unable
    to parse

    Parameters
    ----------
    value: FITS header string in rectange format

    Returns
    -------
    4 member array
    """
    return [float(q) for p in value[1:-1].split(',') for q in p.split(':')]


def ccdsum(value):
    """
    Parses CCDSUM value to get image bin.
    """
    return [float(q) for q in value.split(' ')]

def scale_offset(detsec, shape):
    """
    Returns scale factor and offset for linear amplifier
    to physical transformations
    """
    w = detsec[1] - detsec[0]
    h = detsec[3] - detsec[2]
    scale = (w / shape[1], h / shape[0])
    offs = (
        detsec[0] if w > 0 else detsec[1],
        detsec[2] if h > 0 else detsec[3]
    )
    return scale, offs
