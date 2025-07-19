# Making Maps 

So we use PBF files more info here https://wiki.openstreetmap.org/wiki/PBF_Format  
The OSM PBF is the one we found to be the best 


To Download the file you can go to https://planet.openstreetmap.org  
If you want to view everything go  https://www.openstreetmap.org  
If you want to only download a specific part go https://download.geofabrik.de  

You can also download the sepefici are with osmium  https://osmcode.org/osmium-tool   
using this command ```osmium extract -b min_lon,min_lat,max_lon,max_lat input.osm.pbf -o output.osm.pbf```


Pre setup .osm.pfb files use  
tilemaker https://github.com/systemed/tilemaker

install steps https://github.com/systemed/tilemaker/blob/master/docs/INSTALL.md  
install ***apt install Lua5.1*** in ubuntu then make  

to output the Files we need in z/y/x format pbf files  
``` ./tilemaker ../path/to/.osm.pbf ../path/to/folder/ --store /tmp/store```  
```./tilemaker --input input.osm.pbf --bbox min_lon,min_lat,max_lon,max_lat --output tiles/```  

**Example**
Download north-america-latest.osm.pbf form https://download.geofabrik.d  
use **tilemaker**
```
./tilemaker north-america-latest.osm.pbf  --output tiles/
```
recommend add ```--store /tmp ``` if large file  
copy the content of the tiles/ folder into the SD card maps/ folder

install into the SD card



# Other info
## Other Format you might see mbtiles https://wiki.openstreetmap.org/wiki/MBTiles
you can Generate own for your own Projects (the Badge does not use mbtiles)  
run  
``` ./tilemaker /path/to/.osm.pfb /path/to/.mbtiles ```  
recommend add ```--store /tmp ``` if large file  

### Extract to badge format if you find a MBtile file you want to use  
git clone https://github.com/mapbox/mbutil  
cd mbutil  
```./mb-uitl --image_format=pbf /path/to/.mbiles /path/to/folder/``` to z/x/y.osm.pfb  
if python error and pyton installed run ```sudo ln -s /usr/bin/python3 /usr/bin/python```

Helpful Link for map Tiles:
https://www.reddit.com/r/openstreetmap/comments/yvdj9w/can_i_render_tiles_directly_from_osmpbf_data/
