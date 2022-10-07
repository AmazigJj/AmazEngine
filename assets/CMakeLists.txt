

#set(ASSETS_IMAGES container.png container_specular.png glowstone.png glowstone_emissive.png glowstone_specular.png lighting_maps_specular_color.png
#	lost_empire-Alpha.png lost_empire-RGB.png lost_empire-RGBA.png viking_room.png)

#set(ASSETS_OBJ lost_empire.obj monkey_flat.obj monkey_smooth.obj test.obj viking_room.obj dust2.obj)


#set(ASSETS_MTL lost_empire.mtl monkey_flat.mtl monkey_smooth.mtl test.mtl)

set(ASSETS_OBJ test.obj viking_room.obj dust2.obj)
set(ASSETS_MTL test.mtl dust2.mtl)
set(ASSETS_JSON test.json dust2.json)
set(ASSETS_IMAGES viking_room.png)

add_assets(AmazEngineAssets ${ASSETS_IMAGES} ${ASSETS_OBJ} ${ASSETS_MTL} ${ASSETS_JSON})