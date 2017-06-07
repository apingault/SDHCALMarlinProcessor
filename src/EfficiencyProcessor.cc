#include "EfficiencyProcessor.h"
#include <iostream>

#include <EVENT/LCCollection.h>
#include <EVENT/MCParticle.h>
#include <EVENT/LCParameters.h>
#include <UTIL/CellIDDecoder.h>

// ----- include for verbosity dependend logging ---------
#include "marlin/VerbosityLevels.h"
#include <string.h>
using namespace lcio ;
using namespace marlin ;


EfficiencyProcessor aEfficiencyProcessor ;

EfficiencyProcessor::EfficiencyProcessor() : Processor("EfficiencyProcessor")
{

	// modify processor description
	_description = "EfficiencyProcessor calculates a SDHCAL efficiency and pad multiplcity for each SHDCAL layer" ;


	std::vector<std::string> hcalCollections;
	hcalCollections.push_back(std::string("HCALBarrel"));
	registerInputCollections( LCIO::CALORIMETERHIT,
							  "CollectionName" ,
							  "HCAL Collection Names"  ,
							  _hcalCollections  ,
							  hcalCollections);

	registerProcessorParameter( "RootFileName" ,
								"File name for the root output",
								outputRootName,
								std::string("toto.root") ) ;

	registerProcessorParameter( "NActiveLayers" ,
								"Number of active layers",
								_nActiveLayers,
								48 );

	registerProcessorParameter( "N_ASIC" ,
								"Number of ASIC per layer in x direction",
								_nAsicX,
								int(12) ) ;

	registerProcessorParameter( "N_ASIC" ,
								"Number of ASIC per layer in y direction",
								_nAsicY,
								int(12) ) ;

	int difTab[] = { 181,94,30, 174,175,176, 158,142,141, 129,118,119, 164,152,151,  74,61,75,
					 156,111,110, 102,177,103,  133,136,134,  128,120,121,  65,64,58,  148,72,73,
					 78,79,60,  44,43,113,  243,242,241,   186,127,154,  147,70,71,   47,139,140,
					 143,77,76,   159,91,36,   179,178,183,  41,42,67,  137,46,138,  131,173,144,
					 189,184,160,  172,167,171,  146,135,145,  185,170,180,  187,188,190,  169,165,166,
					 155,57,50,  153,108,25,   51,56,109,   107,150,116,  126,124,49,  117,149,115,
					 48,45,114,   98,93,40,   92,97,100,  62,106,132,  101,35,99,  122,123,130,
					 163,161,162,  104,29,112,  59,53,54,  96,90,27,  95,8,5,  63,87,18 } ;
	std::vector<int> difVec(difTab, difTab + sizeof(difTab) / sizeof(int) ) ;

	registerProcessorParameter( "DifList" ,
								"Vector of dif number",
								_difList,
								difVec ) ;


	std::vector<float> vec ;
	vec.push_back(499.584f) ;
	vec.push_back(499.584f) ;
	vec.push_back(0.0f) ;
	// registerProcessorParameter( "PositionShift" ,
	// 			      "3 Vector to shift to have the right (0,0,0) position",
	// 			      _posShift,
	// 			      vec );
	posShift = CLHEP::Hep3Vector( 0.0 , 0.0 , 0.0 ) ;


	float thr[] = { 1.0f , 2.0f , 3.0f } ; //default value for semi-digital hits
	std::vector<float> thresholdsVec(thr, thr + sizeof(thr) / sizeof(float) ) ;

	std::sort( thresholdsVec.begin() , thresholdsVec.end() ) ;

	registerProcessorParameter( "Thresholds" ,
								"Vector of thresholds",
								thresholdsFloat,
								thresholdsVec ) ;

	AlgorithmRegistrationParameters() ;
}


void EfficiencyProcessor::AlgorithmRegistrationParameters()
{
	/*------------algorithm::Cluster------------*/
	registerProcessorParameter( "Cluster::MaxTransversalCellID" ,
								"Maximum difference between two hits cellID (0 and 1) to build a cluster",
								m_ClusterParameterSetting.maxTransversal,
								1 );

	registerProcessorParameter( "Cluster::MaxLongitudinalCellID" ,
								"Maximum difference between two hits cellID (2) to build a cluster",
								m_ClusterParameterSetting.maxLongitudinal,
								0 );

	registerProcessorParameter( "Cluster::UseDistanceInsteadCellID" ,
								"Boolean to know if clustering algorithms uses distance instead of cellID to cluster hits together",
								m_ClusterParameterSetting.useDistanceInsteadCellID,
								false );

	registerProcessorParameter( "Cluster::MaxTransversalDistance" ,
								"Maximum transversal distance (in mm) between two hits to gathered them in one cluster",
								m_ClusterParameterSetting.maxTransversalDistance,
								11.0f );

	registerProcessorParameter( "Cluster::MaxLongitudinalDistance" ,
								"Maximum longitudinal distance (in mm) between two hits to gathered them in one cluster",
								m_ClusterParameterSetting.maxLongitudinalDistance,
								27.0f );

	/*------------algorithm::ClusteringHelper------------*/
	registerProcessorParameter( "ClusteringHelper::LongitudinalDistanceForIsolation" ,
								"Minimum longitudinal distance (in mm) between one hits and its neighbours to decide if it is isolated",
								m_ClusteringHelperParameterSetting.longitudinalDistance,
								100.0f );

	registerProcessorParameter( "ClusteringHelper::TransversalDistanceDistanceForIsolation" ,
								"Minimum transversal distance (in mm) between one hits and its neighbours to decide if it is isolated",
								m_ClusteringHelperParameterSetting.transversalDistance,
								200.0f );

	/*------------algorithm::Tracking-----------*/
	registerProcessorParameter( "Tracking::ChiSquareLimit" ,
								"Maximum value of tracking fit chi2 to construct a track",
								m_TrackingParameterSetting.chiSquareLimit,
								100.0f );

	registerProcessorParameter( "Tracking::MaxTransverseRatio" ,
								"Maximum value of transverse ratio to construct a track",
								m_TrackingParameterSetting.maxTransverseRatio,
								0.05f );

	registerProcessorParameter( "Tracking::CosThetaLimit" ,
								"Minimum value of cos(Theta) to accept the track",
								m_TrackingParameterSetting.cosThetaLimit,
								0.0f );

	registerProcessorParameter( "Tracking::PrintDebug" ,
								"Boolean to know if debug if printed",
								m_TrackingParameterSetting.printDebug,
								false );

	/*------------algorithm::Efficiency-----------*/
	registerProcessorParameter( "Efficiency::MaxRadius" ,
								"Maximum distance parameter to find a hit to consider the layer as efficient",
								m_EfficiencyParameterSetting.maxRadius,
								25.0f );

	registerProcessorParameter( "Efficiency::SDHCALReadout" ,
								"Boolean to know if the detector used the semi digital readout",
								m_EfficiencyParameterSetting.semiDigitalReadout,
								true );

	registerProcessorParameter( "Efficiency::PrintDebug" ,
								"If true, Efficiency algorithm will print some debug information",
								m_EfficiencyParameterSetting.printDebug,
								false );

	m_EfficiencyParameterSetting.trackingParams=m_TrackingParameterSetting;

	/*------------algorithm::InteractionFinder-----------*/
	registerProcessorParameter( "InteractionFinder::MinSize" ,
								"Minimum cluster size for to define an interaction point",
								m_InteractionFinderParameterSetting.minSize,
								4 );

	registerProcessorParameter( "InteractionFinder::MaxRadius" ,
								"Maximum transversal distance to look for clusters",
								m_InteractionFinderParameterSetting.maxRadius,
								50.0f );

	registerProcessorParameter( "InteractionFinder::MaxDepth" ,
								"Maximum depth (number of layers) to look for clusters",
								m_InteractionFinderParameterSetting.maxDepth,
								4 );

	registerProcessorParameter( "InteractionFinder::MinNumberOfCluster" ,
								"Minimum number of found clusters (big enough) after the interaction point",
								m_InteractionFinderParameterSetting.minNumberOfCluster,
								3 );

	registerProcessorParameter( "InteractionFinder::UseAnalogEnergy" ,
								"Boolean to know if interaction finder algo should use cluster energy of cluster number of hits",
								m_InteractionFinderParameterSetting.useAnalogEnergy,
								false );

	registerProcessorParameter( "InteractionFinder::PrintDebug" ,
								"Boolean to know if debug if printed",
								m_InteractionFinderParameterSetting.printDebug,
								false );
	/*------------caloobject::CaloGeom------------*/
	registerProcessorParameter( "Geometry::NLayers" ,
								"Number of layers",
								m_CaloGeomSetting.nLayers,
								48 );
	registerProcessorParameter( "Geometry::NPixelsPerLayer" ,
								"Number of pixels per layer (assume square geometry)",
								m_CaloGeomSetting.nPixelsPerLayer,
								96 );
	registerProcessorParameter( "Geometry::PixelSize" ,
								"Pixel size (assume square pixels)",
								m_CaloGeomSetting.pixelSize,
								10.408f );

	std::vector<float> vec;
	vec.push_back(10.408f) ;
	vec.push_back(97*10.408f) ;
	registerProcessorParameter( "Geometry::DetectorTransverseSize" ,
								"Define the detector transverse size used by efficiency algorithm (vector size must be 2 or 4; if 2 -> first value is min, second value is max; if 4 -> two first values define x edges , two last values define y edges) ",
								edges,
								vec );
	if( edges.size()==2 ){
		m_CaloGeomSetting.xmin=edges[0];
		m_CaloGeomSetting.ymin=edges[0];
		m_CaloGeomSetting.xmax=edges[1];
		m_CaloGeomSetting.ymax=edges[1];
	}
	else if( edges.size()==4 ){
		m_CaloGeomSetting.xmin=edges[0];
		m_CaloGeomSetting.xmax=edges[1];
		m_CaloGeomSetting.ymin=edges[2];
		m_CaloGeomSetting.ymax=edges[3];
	}
	else{
		std::cout << "WARING : Wrong number of values in paramater Geometry::DetectorTransverseSize => will use default value -500.0, +500.0" << std::endl;
	}
	/*--------------------------------------------*/

	/*------------algorithm::AsicKeyFinder-----------*/
	std::vector<int> asicKeyFactor;
	asicKeyFactor.push_back(1);
	asicKeyFactor.push_back(12);
	asicKeyFactor.push_back(1000);
	registerProcessorParameter( "AsicKeyFinder::KeyFactor" ,
								"Define the factor to build asic keys; default for sdhcal : 1000*K + 12*J + I",
								m_AsicKeyFinderParameterSetting.keyFactors,
								asicKeyFactor );

	registerProcessorParameter( "AsicKeyFinder::NPadX" ,
								"Number of pads in x direction per layer",
								m_AsicKeyFinderParameterSetting.nPadX,
								96 );

	registerProcessorParameter( "AsicKeyFinder::NPadY" ,
								"Number of pads in x direction per layer",
								m_AsicKeyFinderParameterSetting.nPadY,
								96 );

	registerProcessorParameter( "AsicKeyFinder::AsicNPad" ,
								"number of pads in x or y direction per asic (assuming a square)",
								m_AsicKeyFinderParameterSetting.asicNPad,
								8 );

	registerProcessorParameter( "AsicKeyFinder::LayerGap" ,
								"Gap size (in mm) between 2 layers",
								m_AsicKeyFinderParameterSetting.layerGap,
								26.131f );

	registerProcessorParameter( "AsicKeyFinder::PadSize" ,
								"Size of one pad in mm",
								m_AsicKeyFinderParameterSetting.padSize,
								10.408f );

	registerProcessorParameter( "AsicKeyFinder::PrintDebug" ,
								"Boolean to know if debug if printed",
								m_AsicKeyFinderParameterSetting.printDebug,
								false );
}

void EfficiencyProcessor::init()
{
	std::sort(thresholdsFloat.begin() , thresholdsFloat.end() ) ;

	thresholds.clear() ;
	for ( unsigned int i = 0 ; i < thresholdsFloat.size() ; ++i )
		thresholds.push_back( thresholdsFloat.at(i) ) ;


	printParameters() ;

	file = new TFile(outputRootName.c_str() ,"RECREATE") ;

	tree = dynamic_cast<TTree*>( file->Get("tree") ) ;
	if ( !tree )
	{
		std::cout << "tree creation" << std::endl ;
		tree = new TTree("tree" , "Shower variables") ;
	}

	tree->Branch("LayerID" , &layerID) ;
	tree->Branch("DifID" , &difID) ;
	tree->Branch("AsicID" , &asicID) ;
	tree->Branch("PadID" , &padID) ;

	tree->Branch("Efficiencies" , "std::vector<double>" , &efficiencies ) ;
	tree->Branch("EfficienciesError" , "std::vector<double>" , &efficienciesError ) ;


	tree->Branch("Multiplicity" , &multiplicity) ;
	tree->Branch("MultiplicityError" , &multiplicityError) ;
	tree->Branch("Ntrack" , &nTracks) ;
	tree->Branch("Position" , &position) ;

	trackPositionHist = new TH2D("trackPosition" , "trackPosition" , 1010 , 0 , 1010 , 1010 , 0 , 1010) ;

	_nRun = 0 ;
	_nEvt = 0 ;
	_goodTrackCounter = 0 ;

	/*--------------------Algorithms initialisation--------------------*/
	algo_Cluster=new algorithm::Cluster();
	algo_Cluster->SetClusterParameterSetting(m_ClusterParameterSetting);

	algo_ClusteringHelper=new algorithm::ClusteringHelper();
	algo_ClusteringHelper->SetClusteringHelperParameterSetting(m_ClusteringHelperParameterSetting);

	algo_Tracking=new algorithm::Tracking();
	algo_Tracking->SetTrackingParameterSetting(m_TrackingParameterSetting);

	algo_InteractionFinder=new algorithm::InteractionFinder();
	algo_InteractionFinder->SetInteractionFinderParameterSetting(m_InteractionFinderParameterSetting);

	m_EfficiencyParameterSetting.geometry=m_CaloGeomSetting;
	algo_Efficiency = new algorithm::Efficiency() ;
	algo_Efficiency->SetEfficiencyParameterSetting(m_EfficiencyParameterSetting) ;

	algo_AsicKeyFinder=new algorithm::AsicKeyFinder();
	algo_AsicKeyFinder->SetAsicKeyFinderParameterSetting(m_AsicKeyFinderParameterSetting) ;

	for ( unsigned int k = 0 ; k < static_cast<unsigned int>(_nActiveLayers) ; k++ )
	{
		caloobject::Layer* aLayer = new caloobject::SDHCALLayer(static_cast<int>(k) , _difList.at(3*k+2) , _difList.at(3*k+1) , _difList.at(k)) ;
		aLayer->setPosition( CLHEP::Hep3Vector(10.408 , 10.408 , (k+1)*m_AsicKeyFinderParameterSetting.layerGap) ) ;
		aLayer->buildAsics() ;
		aLayer->setThresholds(thresholds) ;
		layers.push_back(aLayer) ;
	}

}

void EfficiencyProcessor::processRunHeader( LCRunHeader* run )
{
	_nRun++ ;
	_nEvt = 0 ;
}

void EfficiencyProcessor::DoTracking()
{
//	std::cout << "DoTracking()" << std::endl ;

	std::vector<caloobject::CaloCluster2D*> clusters ;
	for(std::map<int,std::vector<caloobject::CaloHit*> >::iterator it = hitMap.begin() ; it != hitMap.end() ; ++it)
		algo_Cluster->Run(it->second , clusters) ;

	std::sort(clusters.begin(), clusters.end(), algorithm::ClusteringHelper::SortClusterByLayer) ;
	for(std::vector<caloobject::CaloCluster2D*>::iterator it = clusters.begin() ; it != clusters.end() ; ++it)
	{
		if ( algo_ClusteringHelper->IsIsolatedCluster(*it,clusters) )
		{
			delete *it ;
			clusters.erase(it) ;
			it-- ;
		}
	}
	caloobject::CaloTrack* track = NULL ;
	algo_Tracking->Run(clusters,track) ;

	if( NULL != track )
	{
		algo_InteractionFinder->Run(clusters , track->getTrackParameters()) ;

		if( algo_InteractionFinder->FindInteraction() == false )
		{
			_goodTrackCounter++ ;
			LayerProperties(clusters) ;
		}
	}
	file->cd() ;

	delete track ;
	for ( std::vector<caloobject::CaloCluster2D*>::iterator it = clusters.begin() ; it != clusters.end() ; ++it )
		delete (*it) ;
}

void EfficiencyProcessor::LayerProperties(std::vector<caloobject::CaloCluster2D*> &clusters)
{
//	std::cout << "LayerProperties()" << std::endl ;

	int trackBegin = (*clusters.begin())->getLayerID() ;
	int trackEnd = (*(clusters.end()-1))->getLayerID() ;
	if ( trackBegin == 1 )
		trackBegin = 0 ;

	if ( trackEnd == _nActiveLayers-2 )
		trackEnd = _nActiveLayers-1 ;


	for( int K = trackBegin ; K <= trackEnd ; K++ )
	{
//		std::cout << "K : " << K << std::endl ;
//		std::cout << "toto" << std::endl ;
		algo_Efficiency->Run(layers.at(K) , clusters) ;
//		std::cout << "titi" << std::endl ;

		if ( algo_Efficiency->isTrack() )
		{
			layers.at(K)->update( algo_Efficiency->getExpectedPosition() , algo_Efficiency->getGoodCluster() ) ;
			trackPositionHist->Fill( algo_Efficiency->getExpectedPosition().x() , algo_Efficiency->getExpectedPosition().y() ) ;
		}
	}
}

void EfficiencyProcessor::processEvent( LCEvent * evt )
{
	//
	// * Reading HCAL Collections of CalorimeterHits*
	//

	UTIL::CellIDDecoder<EVENT::CalorimeterHit> IDdecoder("M:3,S-1:3,I:9,J:9,K-1:6");

	for (unsigned int i(0); i < _hcalCollections.size(); ++i)
	{
		std::string colName =  _hcalCollections[i] ;
		try
		{
			col = evt->getCollection( _hcalCollections[i].c_str() ) ;
			//initString = col->getParameters().getStringVal(LCIO::CellIDEncoding);
			numElements = col->getNumberOfElements();
			//      UTIL::CellIDDecoder<CalorimeterHit*> idDecoder(col);
			for (int j=0; j < numElements; ++j)
			{
				CalorimeterHit * hit = dynamic_cast<CalorimeterHit*>( col->getElementAt( j ) ) ;
				CLHEP::Hep3Vector vec(hit->getPosition()[0],hit->getPosition()[1],hit->getPosition()[2]);
				int cellID[] = {static_cast<int>(IDdecoder(hit)["I"]),
												static_cast<int>(IDdecoder(hit)["J"]),
												static_cast<int>(IDdecoder(hit)["K-1"])
											} ;

				if ( cellID[2] >= _nActiveLayers )
					continue ;

				if ( cellID[0] < 1 || cellID[0] > 96 || cellID[1] < 1 || cellID[1] > 96 )
					continue ;

				if (hit->getEnergy() < thresholds.at(0) )
					continue ;

				caloobject::CaloHit* aHit = new caloobject::CaloHit(cellID,vec,hit->getEnergy(),hit->getTime() , posShift) ;
				hitMap[cellID[2]].push_back(aHit) ;
			}

			DoTracking() ;
			clearVec() ;
		}
		catch(DataNotAvailableException &e)
		{
			std::cout << "Exception " << std::endl;
		}
	}
	_nEvt ++ ;
	std::cout << "Event processed : " << _nEvt << " -------------------------------------------------------------------------------" << std::endl;
}

void EfficiencyProcessor::clearVec()
{
	for(std::map<int,std::vector<caloobject::CaloHit*> >::iterator it = hitMap.begin() ; it != hitMap.end() ; ++it)
		for( std::vector<caloobject::CaloHit*>::iterator jt = (it->second).begin() ; jt != (it->second).end() ; ++jt)
			delete *(jt) ;

	hitMap.clear( ) ;
}


void EfficiencyProcessor::check( LCEvent * /*evt*/ )
{
	// nothing to check here - could be used to fill checkplots in reconstruction processor
}


void EfficiencyProcessor::end()
{
	file->cd() ;

	file->WriteObject( &thresholds , "Thresholds" ) ;

	int globalNTrack = 0 ;
	std::vector<double> globalEff(thresholds.size() , 0.0) ;
	std::vector<double> globalEffErr(thresholds.size() , 0.0) ;

	double globalMul = 0 ;
	double globalMulErr = 0 ;
	int nOkLayersForMul = 0 ;

	for( std::vector<caloobject::Layer*>::const_iterator layIt = layers.begin() ; layIt != layers.end() ; ++layIt )
	{
		layerID = (*layIt)->getID() ;

		//test
//		if ( layerID == 1 || layerID == 34 )
//			continue ;

		AsicMap asics = (*layIt)->getAsics() ;
		for( AsicMap::const_iterator asicIt = asics.begin() ; asicIt != asics.end() ; ++asicIt )
		{
			difID = asicIt->second->getDifID() ;
			asicID = asicIt->second->getID() ;

			PadMap pads = asicIt->second->getPads() ;
			for( PadMap::const_iterator padIt = pads.begin() ; padIt != pads.end() ; ++padIt )
			{
				padID = padIt->second->getID() ;

				nTracks = padIt->second->getNTrack() ;

				efficiencies = padIt->second->getEfficiencies() ;
				efficienciesError = padIt->second->getEfficienciesError() ;

				multiplicity = padIt->second->getMultiplicity() ;
				multiplicityError = padIt->second->getMultiplicityError() ;

				position.clear() ;
				position.push_back( padIt->second->getPosition().x() ) ;
				position.push_back( padIt->second->getPosition().y() ) ;
				position.push_back( padIt->second->getPosition().z() ) ;

				tree->Fill() ;

			} //pad loop

			//glocal values for asic
			padID = -1 ;

			nTracks = asicIt->second->getNTrack() ;

			efficiencies = asicIt->second->getEfficiencies() ;
			efficienciesError = asicIt->second->getEfficienciesError() ;

			multiplicity = asicIt->second->getMultiplicity() ;
			multiplicityError = asicIt->second->getMultiplicityError() ;

			position.clear() ;
			position.push_back( asicIt->second->getPosition().x() ) ;
			position.push_back( asicIt->second->getPosition().y() ) ;
			position.push_back( asicIt->second->getPosition().z() ) ;

			tree->Fill() ;

		} //asic loop

		//glocal values for layer
		asicID = -1 ;
		padID = -1 ;
		difID = -1 ;

		nTracks = (*layIt)->getNTrack() ;

		efficiencies = (*layIt)->getEfficiencies() ;
		efficienciesError = (*layIt)->getEfficienciesError() ;

		multiplicity = (*layIt)->getMultiplicity() ;
		multiplicityError = (*layIt)->getMultiplicityError() ;

		position.clear() ;
		position.push_back( (*layIt)->getPosition().x() ) ;
		position.push_back( (*layIt)->getPosition().y() ) ;
		position.push_back( (*layIt)->getPosition().z() ) ;

		tree->Fill() ;

		if ( efficiencies.at(0) > 0 )
		{
			globalMul += multiplicity ;
			globalMulErr += 1.0/(multiplicityError*multiplicityError) ;
			nOkLayersForMul++ ;
		}

		globalNTrack += nTracks ;

		for ( unsigned int i = 0 ; i < thresholds.size() ; ++i )
		{
			globalEff.at(i) += efficiencies.at(i) ;
			globalEffErr.at(i) += 1.0/( efficienciesError.at(i)*efficienciesError.at(i) ) ;
		}

		std::cout << "Layer " << layerID << "        mul : " << multiplicity << " +- " << multiplicityError << std::endl ;

	} //layer loop

	globalMul /= nOkLayersForMul ;

	std::cout << "Global mulPerLayer : " << globalMul << std::endl ;

	//global
	layerID = -1 ;
	difID = -1 ;
	asicID = -1 ;
	padID = -1 ;

	nTracks = globalNTrack ;

	efficiencies = std::vector<double>( thresholds.size() , 0.0 ) ;
	efficienciesError = std::vector<double>( thresholds.size() , 0.0 ) ;

	for ( unsigned int i = 0 ; i < thresholds.size() ; ++i )
	{
		efficiencies.at(i) = ( globalEff.at(i)/layers.size() ) ;
//		efficiencies.at(i) = ( globalEff.at(i)/(layers.size()-2) ) ;
		efficienciesError.at(i) = std::sqrt( 1.0/globalEffErr.at(i) ) ;
	}


	multiplicity = globalMul ;
	multiplicityError = std::sqrt(1.0/globalMulErr) ;


	position.clear() ;
	position.push_back( layers.at(0)->getPosition().x() ) ;
	position.push_back( layers.at(0)->getPosition().y() ) ;
	position.push_back( layers.at(0)->getPosition().z() ) ;


	tree->Fill() ;


	delete algo_Cluster ;
	delete algo_ClusteringHelper ;
	delete algo_Tracking ;
	delete algo_InteractionFinder ;
	delete algo_Efficiency ;
	delete algo_AsicKeyFinder ;

	for(std::vector<caloobject::Layer*>::iterator it = layers.begin() ; it != layers.end() ; ++it)
		delete (*it) ;
	layers.clear() ;

	file->Write() ;

	file->Purge() ;
	file->Close() ;

	std::cout << "_goodTrackCounter " << _goodTrackCounter << std::endl ;
}
