/* Copyright (C) 2010 Ion Torrent Systems, Inc. All Rights Reserved */
#include "BeadfindControlOpts.h"
#include "Utils.h"
#include "string.h"

void BeadfindControlOpts::DefaultBeadfindControl()
{
  //maxNumKeyFlows = 0;
  //minNumKeyFlows = 99;
  bfMinLiveRatio = .0001;
  bfMinLiveLibSnr = 3;
  bfMinLiveTfSnr = -1;
  bfTfFilterQuantile = 1;
  bfLibFilterQuantile = 1;
  skipBeadfindSdRecover = 1;
  beadfindThumbnail = 0;
  beadfindLagOneFilt = 0;
  beadMaskFile = NULL;
  maskFileCategorized = false;
  sprintf (bfFileBase, "beadfind_post_0003.dat");
  sprintf (preRunbfFileBase, "beadfind_pre_0003.dat");
  BF_ADVANCED = true;
  SINGLEBF = true;
  noduds = false;
  beadfindUseSepRef = false;
  numThreads = -1;
  minTfPeakMax = 40.0f;
  minLibPeakMax = 15.0f;
  bfOutputDebug = 0;
  bfMult = 1.0;
  sdAsBf = true;
  gainCorrection = true;
  useSignalReference = 4;
  useSignalReferenceSet = false;
  blobFilter = false;
  if (isInternalServer()) {
    bfOutputDebug = 2;
  }
  beadfindType = "differential";
  predictFlowStart = -1;
  predictFlowEnd = -1;
}

BeadfindControlOpts::~BeadfindControlOpts()
{
  if (beadMaskFile)
    free (beadMaskFile);
}

void BeadfindControlOpts::PrintHelp()
{
	printf ("     BeadfindControlOpts\n");
    printf ("  -b,--beadfindfile          FILE              prerun bf file base []\n");
    printf ("  -b,--beadfindFile          FILE              same as --beadfindfile []\n");
    printf ("     --beadfind-type         STRING            beadfind type [naoh,positive]\n");
	printf ("     --use-beadmask          FILE              bead mask file name []\n");
	printf ("     --beadmask-categorized  BOOL              beadmask categorized [false]\n");
	printf ("     --beadfind-basis        STRING            beadfind basis []\n");
	printf ("     --beadfind-dat          STRING            beadfind dat []\n");
	printf ("     --beadfind-bgdat        STRING            beadfind bgdat []\n");
	printf ("     --beadfind-sdasbf       BOOL              beadfind sdAsBf [true]\n");
	printf ("     --beadfind-bfmult       FLOAT             beadfind mult [1.0]\n");
	printf ("     --beadfind-minlive      DOUBLE            beadfind minlive ratio [0.0001]\n");
	printf ("     --beadfind-minlivesnr   DOUBLE            beadfind min live lib SNR [4.0]\n");
	printf ("     --beadfind-min-tf-snr   DOUBLE            beadfind min live tf SNR [-1]\n");
	printf ("     --beadfind-tf-min-peak  FLOAT             beadfind min tf peakmMax [40.0]\n");
	printf ("     --beadfind-lib-min-peak FLOAT             beadfind min lib peakmMax [15.0]\n");
	printf ("     --beadfind-lib-filt     DOUBLE            beadfind lib filter quantile [1.0]\n");
	printf ("     --beadfind-tf-filt      DOUBLE            beadfind tf filter quantile [1.0]\n");
	printf ("     --beadfind-skip-sd-recover          INT   beadfind skip beadfind sd recover [1]\n");
	printf ("     --beadfind-thumbnail    INT               beadfind thumbnail [0]\n");
	printf ("     --beadfind-sep-ref      BOOL              beadfind use seperated ref [false]\n");
	printf ("     --beadfind-lagone-filt  INT               beadfind lagone filt [0]\n");
	printf ("     --beadfind-diagnostics  INT               beadfind output debug [0]\n");
	printf ("     --beadfind-washout      BOOL              beadfind washout [false]\n");
	printf ("     --beadfind-gain-correction          BOOL  beadfind gain correction [true]\n");
	printf ("     --beadfind-blob-filter  BOOL              beadfind blob filter [false]\n");
	printf ("     --beadfind-predict-start            INT   beadfind predict flow start [-1]\n");
	printf ("     --beadfind-predict-end  INT               beadfind predict flow end [-1]\n");
	printf ("     --beadfind-sig-ref-type INT               beadfind use signal reference []\n");
	printf ("     --beadfind-zero-flows   STRING            beadfind double tap flows []\n");
	printf ("     --beadfind-num-threads  INT               beadfind number of threads [-1]\n");
	printf ("     --bfold                 BOOL              BF_ADVANCED [true]\n");
	printf ("     --noduds                BOOL              noduds [false]\n");
    printf ("\n");
}

void BeadfindControlOpts::SetOpts(OptArgs &opts, Json::Value& json_params)
{
  beadfindType = RetrieveParameterString(opts, json_params, '-', "beadfind-type", "differential");
  if ( beadfindType != "differential" )
     {
      fprintf ( stderr, "*Error* - Illegal option to --beadfind-type: %s, valid options are 'differential'\n",
                 beadfindType.c_str() );
      exit ( EXIT_FAILURE );
     }
	string s1 = RetrieveParameterString(opts, json_params, '-', "use-beadmask", "");
	if(s1.length() > 0)
	{
		sprintf(beadMaskFile, "%s", s1.c_str());
	}
	maskFileCategorized = RetrieveParameterBool(opts, json_params, '-', "beadmask-categorized", false);
	string s2 = RetrieveParameterString(opts, json_params, '-', "beadfind-basis", "naoh");
	if(s2.length() > 0)
	{
		bfType = s2;
		if ( bfType != "naoh" && bfType != "positive"  && bfType != "nobuffer")
		{
			fprintf ( stderr, "*Error* - Illegal option to --beadfind-basis: %s, valid options are 'naoh','nobuffer' or 'positive'\n",	bfType.c_str() );
			exit ( EXIT_FAILURE );
		}
	}
	bfDat = RetrieveParameterString(opts, json_params, '-', "beadfind-dat", "beadfind_pre_0003.dat");
	bfBgDat = RetrieveParameterString(opts, json_params, '-', "beadfind-bgdat", "");
	sdAsBf = RetrieveParameterBool(opts, json_params, '-', "beadfind-sdasbf", true);
	bfMult = RetrieveParameterFloat(opts, json_params, '-', "beadfind-bfmult", 1.0);
	bfMinLiveRatio = RetrieveParameterDouble(opts, json_params, '-', "beadfind-minlive", 0.0001);
	bfMinLiveLibSnr = RetrieveParameterDouble(opts, json_params, '-', "beadfind-minlivesnr", 4.0);
	bfMinLiveTfSnr = RetrieveParameterDouble(opts, json_params, '-', "beadfind-min-tf-snr", -1);
	minTfPeakMax = RetrieveParameterFloat(opts, json_params, '-', "beadfind-tf-min-peak", 40.0);
	minLibPeakMax = RetrieveParameterFloat(opts, json_params, '-', "beadfind-lib-min-peak", 15.0);
	bfLibFilterQuantile = RetrieveParameterDouble(opts, json_params, '-', "beadfind-lib-filt", 1.0);
	bfTfFilterQuantile = RetrieveParameterDouble(opts, json_params, '-', "beadfind-tf-filt", 1.0);
	skipBeadfindSdRecover = RetrieveParameterInt(opts, json_params, '-', "beadfind-skip-sd-recover", 1);
	beadfindThumbnail = RetrieveParameterInt(opts, json_params, '-', "beadfind-thumbnail", 0);
	beadfindUseSepRef = RetrieveParameterBool(opts, json_params, '-', "beadfind-sep-ref", false);
	beadfindLagOneFilt = RetrieveParameterInt(opts, json_params, '-', "beadfind-lagone-filt", 0);
	int defaultVal = 0;
	if (isInternalServer())
	{
		defaultVal = 2;
	}
	bfOutputDebug = RetrieveParameterInt(opts, json_params, '-', "beadfind-diagnostics", defaultVal);
	bool b1 = RetrieveParameterBool(opts, json_params, '-', "bead-washout", false);
	SINGLEBF = !b1;
	gainCorrection = RetrieveParameterBool(opts, json_params, '-', "beadfind-gain-correction", true);
	blobFilter = RetrieveParameterBool(opts, json_params, '-', "beadfind-blob-filter", false);
	predictFlowStart = RetrieveParameterInt(opts, json_params, '-', "beadfind-predict-start", -1);
	predictFlowEnd = RetrieveParameterInt(opts, json_params, '-', "beadfind-predict-end", -1);
	useSignalReference = RetrieveParameterInt(opts, json_params, '-', "beadfind-sig-ref-type", -1);
	if(useSignalReference == -1)
	{
		useSignalReference = 4;
	}
	else
	{
		useSignalReferenceSet = true;
	}
	doubleTapFlows = RetrieveParameterString(opts, json_params, '-', "beadfind-zero-flows", "");
	numThreads = RetrieveParameterInt(opts, json_params, '-', "beadfind-num-threads", -1);

	//jz the following comes from CommandLineOpts::GetOpts
	BF_ADVANCED = RetrieveParameterBool(opts, json_params, '-', "bfold", true);
	noduds = RetrieveParameterBool(opts, json_params, '-', "noduds", false);
	string sb = RetrieveParameterString(opts, json_params, 'b', "beadfindfile", "");
	if(sb.length() > 0)
	{
		snprintf ( preRunbfFileBase, 256, "%s", sb.c_str() );
        bfFileBase[0] = '\0';
        SINGLEBF = true;
	}
}
