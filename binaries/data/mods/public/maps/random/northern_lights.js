RMS.LoadLibrary("rmgen");

var tGrass = ["polar_snow_b"];
var tGrassA = "polar_ice_snow";
var tGrassB = "polar_ice";
var tGrassC = "polar_snow_a";
var tForestFloor = "polar_tundra_snow";
var tCliff = "polar_snow_rocks";
var tDirt = ["polar_snow_glacial"];
var tRoad = "new_alpine_citytile";
var tRoadWild = "new_alpine_citytile";
var tShoreBlend = "alpine_shore_rocks_icy";
var tShore = "alpine_shore_rocks";
var tWater = "alpine_shore_rocks";

// gaia entities
var oTree = "gaia/flora_tree_pine_w";
var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
var oStoneSmall = "gaia/geology_stone_alpine_a";
var oMetalLarge = "gaia/geology_metal_alpine_slabs";
var oFish = "gaia/fauna_fish";
var oDeer = "gaia/fauna_walrus";
var oSheep = "gaia/fauna_wolf_snow";

// decorative props
var aRockLarge = "actor|geology/stone_granite_med.xml";
var aRockMedium = "actor|geology/stone_granite_med.xml";
var aIceberg = "actor|props/special/eyecandy/iceberg.xml";

var pForestD = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor, tForestFloor];
var pForestP = [tForestFloor + TERRAIN_SEPARATOR + oTree, tForestFloor, tForestFloor, tForestFloor];
const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

// create tile classes

var clPlayer = createTileClass();
var clHill = createTileClass();
var clHill2 = createTileClass();
var clHill3 = createTileClass();
var clHill4 = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();


// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerPos = new Array(numPlayers);

for (var i = 0; i < numPlayers; i++)
{
	playerPos[i] = (i + 1) / (numPlayers + 1);
	playerX[i] = playerPos[i];
	playerZ[i] = 0.35 + 0.2*(i%2)
}

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);
	addToClass(ix, iz, clPlayer);
	addToClass(ix+5, iz, clPlayer);
	addToClass(ix, iz+5, clPlayer);
	addToClass(ix-5, iz, clPlayer);
	addToClass(ix, iz-5, clPlayer);
	
	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);
	
	// create starting units
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE);
	
	// create metal mine
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	
	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	var hillSize = PI * radius * radius;
	// create starting trees
	var num = floor(hillSize / 60);
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(12, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oTree, num, num, 0,3)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));
	
}

RMS.SetProgress(15);

// create northern sea
var fadedistance = 8;

for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		
		if (iz > 0.69 * mapSize)
		{
			if (iz < 0.69 * mapSize + fadedistance)
			{
				setHeight(ix, iz, 3 - 8 * (iz - 0.69 * mapSize) / fadedistance);
				if (ix, iz, 3 - 8 * (iz - 0.69 * mapSize) / fadedistance < 0.5)
					addToClass(ix, iz, clWater);
			}
			else
			{
				setHeight(ix, iz, -5);
				addToClass(ix, iz, clWater);
			}
		}
	}
}

// create shore
log("Creating shores...");
for (var i = 0; i < scaleByMapSize(20,120); i++)
{
	placer = new ClumpPlacer(scaleByMapSize(50, 70), 0.2, 0.1, 10, randFloat(0.1,0.9)*mapSize, randFloat(0.67,0.74)*mapSize);
	var terrainPainter = new LayeredPainter(
		[tGrass, tGrass],		// terrains
		[2]								// widths
	);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 3);
	createArea(
		placer,
		[terrainPainter, elevationPainter, unPaintClass(clWater)], 
		null
	);
}

// create islands
log("Creating islands...");
placer = new ClumpPlacer(scaleByMapSize(40, 180), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tGrass, tGrass],		// terrains
	[3]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 3, 3);
createAreas(
	placer,
	[terrainPainter, elevationPainter, unPaintClass(clWater)], 
	stayClasses(clWater, 7),
	scaleByMapSize(10, 80)
);

paintTerrainBasedOnHeight(-6, 1, 1, tWater);

// create lakes
log("Creating lakes...");
var numLakes = round(scaleByMapSize(1,4) * numPlayers);
var placer = new ClumpPlacer(scaleByMapSize(100,250), 0.8, 0.1, 10);
var terrainPainter = new LayeredPainter(
	[tShoreBlend, tShore, tWater],		// terrains
	[1,1]							// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -4, 3);
var waterAreas = createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clWater)], 
	avoidClasses(clPlayer, 15, clWater, 20),
	numLakes
);

paintTerrainBasedOnHeight(1, 2.8, 1, tShoreBlend);
paintTileClassBasedOnHeight(-6, 0.5, 1, clWater)

RMS.SetProgress(45);

// create hills
log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tGrass],		// terrains
	[3]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 25, 3);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)], 
	avoidClasses(clPlayer, 12, clHill, 15, clWater, 2, clBaseResource, 2),
	scaleByMapSize(2, 8) * numPlayers
);


// calculate desired number of trees for map (based on size)

var MIN_TREES = 100;
var MAX_TREES = 625;
var P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

// create forests
log("Creating forests...");
var types = [
	[[tGrass, tGrass, tGrass, tGrass, pForestD], [tGrass, tGrass, tGrass, pForestD]],
	[[tGrass, tGrass, tGrass, tGrass, pForestP], [tGrass, tGrass, tGrass, pForestP]]
];	// some variation


var size = numForest / (scaleByMapSize(2,8) * numPlayers);

var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)], 
		avoidClasses(clPlayer, 12, clForest, 20, clHill, 0, clWater, 8),
		num
	);
}

log("Creating iceberg...");
// create iceberg
group = new SimpleGroup([new SimpleObject(aIceberg, 0,2, 0,4)], true, clRock);
createObjectGroups(group, 0,
	[avoidClasses(clRock, 4), stayClasses(clWater, 4)],
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(70);

// create dirt patches
log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 9), scaleByMapSize(5, 15), scaleByMapSize(8, 24)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tGrassC,tGrassA,tGrassB], 		// terrains
		[2,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80)
	);
}
var sizes = [scaleByMapSize(3, 9), scaleByMapSize(5, 15), scaleByMapSize(8, 24)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[tDirt,tDirt], 		// terrains
		[1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 8, clForest, 0, clHill, 0, clPlayer, 12, clDirt, 16),
		scaleByMapSize(20, 80)
	);
}

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

// create small stone quarries
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 1, clPlayer, 10, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);


RMS.SetProgress(95);

// create straggler trees
log("Creating straggler trees...");
var types = [oTree];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroups(group, 0,
		avoidClasses(clWater, 5, clForest, 1, clHill, 1, clPlayer, 12, clMetal, 1, clRock, 1),
		num
	);
}

// create deer
log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

RMS.SetProgress(75);

// create sheep
log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oSheep, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

// create fish
log("Creating fish...");
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroups(group, 0,
	[avoidClasses(clFood, 20), stayClasses(clWater, 6)],
	25 * numPlayers, 60
);

setSunColour(0.6, 0.6, 0.6);	
setSunElevation(PI/ 6);

setWaterColour(0.0, 0.047, 0.286);				// dark majestic blue
setWaterTint(0.471, 0.776, 0.863);				// light blue
setWaterReflectionTint(0.0, 0.047, 0.286);
setWaterMurkiness(0.82);
setWaterWaviness(3);
setWaterReflectionTintStrength(0.15);

setSkySet("fog");
ExportMap();
