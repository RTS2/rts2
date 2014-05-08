-- auger shooter is optional component
DROP TABLE auger_observation;
DROP TABLE auger;

CREATE TABLE auger (
	auger_t3id	integer PRIMARY KEY NOT NULL,
	auger_date	timestamp NOT NULL,
	ra		float8 NOT NULL,
	dec		float8 NOT NULL,
	cut		integer NOT NULL,
	-- now all fields from message..

	eye		integer NOT NULL, -- FD eye Id
	run		integer NOT NULL, -- FD run number
	event		integer NOT NULL, -- FD event number
	augerid 	varchar(50) NOT NULL, -- Event Auger Id after SD/FD merger
	GPSSec		integer NOT NULL, -- GPS second (SD)
	GPSNSec		integer NOT NULL, -- GPS nano second (SD)
	SDId		integer NOT NULL, -- SD Event Id
	NPix		integer NOT NULL, -- Num. pixels with a pulse after FdPulseFinder
	SDPTheta	float8 NOT NULL,  -- Zenith angle of SDP normal vector (deg)
	SDPThetaErr	float8 NOT NULL,  -- Uncertainty of SDPtheta
	SDPPhi		float8 NOT NULL,  -- Azimuth angle of SDP normal vector (deg)
	SDPPhiErr	float8 NOT NULL,  -- Uncertainty of SDPphi
	SDPChi2		float8 NOT NULL,  -- Chi^2 of SDP fit
	SDPNdf		integer NOT NULL, -- Degrees of freedom of SDP fit
	Rp		float8 NOT NULL,  -- Shower impact parameter Rp (m)
	RpErr		float8 NOT NULL,  -- Uncertainty of Rp (m)
	Chi0		float8 NOT NULL,  -- Angle of shower in the SDP (deg)
	Chi0Err		float8 NOT NULL,  -- Uncertainty of Chi0 (deg)
	T0		float8 NOT NULL,  -- FD time fit T_0 (ns)
	T0Err		float8 NOT NULL,  -- Uncertainty of T_0 (ns)
	TimeChi2	float8 NOT NULL,  -- Full Chi^2 of axis fit
	TimeChi2FD	float8 NOT NULL,  -- Chi^2 of axis fit (FD only)
	TimeNdf		float8 NOT NULL,  -- Degrees of freedom of axis fit
	Easting		float8 NOT NULL,  -- Core position in easting coordinate (m)
	Northing	float8 NOT NULL,  -- Core position in northing coordinate (m)
	Altitude	float8 NOT NULL,  -- Core position altitude (m)
	NorthingErr	float8 NOT NULL,  -- Uncertainty of northing coordinate (m)
	EastingErr	float8 NOT NULL,  -- Uncertainty of easting coordinate (m)
	Theta		float8 NOT NULL,  -- Shower zenith angle in core coords. (deg)
	ThetaErr	float8 NOT NULL,  -- Uncertainty of zenith angle (deg)
	Phi		float8 NOT NULL,  -- Shower azimuth angle in core coords. (deg)
	PhiErr		float8 NOT NULL,  -- Uncertainty of azimuth angle (deg)
	dEdXmax		float8 NOT NULL,  -- Energy deposit at shower max (GeV/(g/cm^2))
	dEdXmaxErr	float8 NOT NULL,  -- Uncertainty of Nmax (GeV/(g/cm^2))
	X_max		float8 NOT NULL,  -- Slant depth of shower maximum (g/cm^2)
	X_maxErr	float8 NOT NULL,  -- Uncertainty of Xmax (g/cm^2)
	X0		float8 NOT NULL,  -- X0 Gaisser-Hillas fit (g/cm^2)
	X0Err		float8 NOT NULL,  -- Uncertainty of X0 (g/cm^2)
	Lambda		float8 NOT NULL,  -- Lambda of Gaisser-Hillas fit (g/cm^2)
	LambdaErr	float8 NOT NULL,  -- Uncertainty of Lambda (g/cm^2)
	GHChi2		float8 NOT NULL,  -- Chi^2 of Gaisser-Hillas fit
	GHNdf		float8 NOT NULL,  -- Degrees of freedom of GH fit
	LineFitChi2	float8 NOT NULL,  -- Chi^2 of linear fit to profile

	EmEnergy	float8 NOT NULL,  -- Calorimetric energy from GH fit (EeV)
	EmEnergyErr	float8 NOT NULL,  -- Uncertainty of Eem (EeV)
	Energy		float8 NOT NULL,  -- Total energy from GH fit (EeV)
	EnergyErr	float8 NOT NULL,  -- Uncertainty of Etot (EeV)

	MinAngle	float8 NOT NULL,  -- Minimum viewing angle (deg)
	MaxAngle	float8 NOT NULL,  -- Maximum viewing angle (deg)
	MeanAngle	float8 NOT NULL,  -- Mean viewing angle (deg)

	NTank		float8 NOT NULL,  -- Number of stations in hybrid fit
	HottestTank	float8 NOT NULL,  -- Station used in hybrid-geometry reco
	AxisDist	float8 NOT NULL,  -- Shower axis distance to hottest station (m)
	SDPDist		float8 NOT NULL,  -- SDP distance to hottest station (m)

	SDFDdT		float8 NOT NULL,  -- SD/FD time offset after the minimization (ns)
	XmaxEyeDist	float8 NOT NULL,  -- Distance to shower maximum (m)
	XTrackMin	float8 NOT NULL,  -- First recorded slant depth of track (g/cm^2)
	XTrackMax	float8 NOT NULL,  -- Last recorded slant depth of track (g/cm^2)
	XFOVMin		float8 NOT NULL,  -- First slant depth inside FOV (g/cm^2)
	XFOVMax		float8 NOT NULL,  -- Last slant depth inside FOV (g/cm^2)
	XTrackObs	float8 NOT NULL,  -- Observed track length depth (g/cm^2)
	DegTrackObs	float8 NOT NULL,  -- Observed track length angle (deg)
	TTrackObs	float8 NOT NULL,  -- Observed track length time (100 ns)
	ShowerParams    varchar(2000),    -- Shower parameters

	-- user settable cut tresholds parameters
	EyeId1		integer, -- Eye Id, cut1
	minEnergy	float8,  -- minimal shower energy, cut1
	maxXmaxErr	float8,  -- Maximal XmaxErr value, cut1
	maxEnergyDiv	float8,  -- Maximal energy div, (EnergyErr / Energy), cut1
	maxGHChiDiv	float8,  -- Maximal GHChi div, (GHChi2 / GHNdf), cut1
	minLineFitDiff	float8,  -- Minimal line fit difference, (LineFitChi2 - GHChi2), cut1
	maxAxisDist	float8,  -- Maximal axis distance, cut1
	minRp		float8,  -- Minimal Rp Value, cut1
	minChi0		float8,  -- Minimal Chi0 value, cut1
	maxSPDDiv	float8,  -- Maximal SPD Div, cut1
	maxTimeDiv	float8,  -- Maximal time difference, cut1
	maxTheta	float8,  -- Maximal Theta value, cut1
	maxTime		integer, -- maximal time between shower and its observations, cut1
	EyeId2		integer, -- Eye Id, cut2
	minEnergy2	float8,  -- minimal shower energy, cut2
	minPix2		integer, -- Minimal number of pixels, cut2
	maxAxisDist2	float8,  -- Maximal axis distance, cut2
	maxTimeDiff2	float8,  -- Maximal FD/SD time difference, cut2
	maxGHChiDiv2	float8,  -- Maximal GHChi div, (GHChi2 / GHNdf), cut2
	maxLineFitDiv2	float8,  -- Minimal line fit difference, (GHChi2 / LineFitChi2), cut2
	minViewAngle2	float8,  -- Minimal viewing angle, cut2
	EyeId3		integer, -- Eye Id, cut3
	minEnergy3	float8,  -- minimal shower energy, cut3
	maxXmaxErr3	float8,  -- Maximal XmaxErr value, cut3
	maxEnergyDiv3	float8,  -- Maximal energy div, (EnergyErr / Energy), cut3
	maxGHChiDiv3	float8,  -- Maximal GHChi div, (GHChi2 / GHNdf), cut3
	maxLineFitDiv3	float8,  -- Minimal line fit difference, (GHChi2 / LineFitChi2), cut3
	minPix3		integer, -- Minimal number of pixels, cut3
	maxAxisDist3	float8,  -- Maximal axis distance, cut3
	minRp3		float8,  -- Minimal Rp Value, cut3
	minChi03	float8,  -- Minimal Chi0 value, cut3
	maxSPDDiv3	float8,  -- Maximal SPD Div, cut3
	maxTimeDiv3	float8,  -- Maximal time difference, cut3
	EyeId4		integer, -- Eye Id, cut4
	minNPix4	integer, -- minimal number of pixels, cut4
	maxDGHChi2Improv4	float8 -- maximal DGHChi2 Improv, cut4
);

-- relation between observations and auger targets is kept in this table
CREATE TABLE auger_observation (
	auger_t3id	integer NOT NULL REFERENCES auger (auger_t3id),
	obs_id		integer NOT NULL REFERENCES observations (obs_id),
CONSTRAINT auger_obs_prim_key PRIMARY KEY (auger_t3id, obs_id)
);

GRANT ALL ON auger TO GROUP observers;
GRANT ALL ON auger_observation TO GROUP observers;
