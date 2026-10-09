"""Run with UE editor Python. Creates native materials, textures, Blueprint and map."""
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir())
assets = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
matlib = unreal.MaterialEditingLibrary
for folder in ('Materials', 'Textures', 'Blueprints', 'Maps'):
    lib.make_directory('/Game/Indigo/' + folder)

tasks = []
for png in sorted((root / 'AssetSources').glob('*.png')):
    task = unreal.AssetImportTask()
    task.filename = str(png)
    task.destination_path = '/Game/Indigo/Textures'
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)
assets.import_asset_tasks(tasks)
for task in tasks:
    for path in task.imported_object_paths:
        texture = lib.load_asset(path)
        if isinstance(texture, unreal.Texture2D):
            # Indigo blue can be mistaken for a normal map by the importer.
            texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_DEFAULT)
            texture.set_editor_property('srgb', True)
            lib.save_loaded_asset(texture)

def material(name):
    path = '/Game/Indigo/Materials/' + name
    existing = lib.load_asset(path) if lib.does_asset_exist(path) else None
    if existing:
        return existing
    return assets.create_asset(name, '/Game/Indigo/Materials', unreal.Material, unreal.MaterialFactoryNew())

surface = material('M_Surface')
matlib.delete_all_material_expressions(surface)
color = matlib.create_material_expression(surface, unreal.MaterialExpressionVectorParameter, -350, 0)
color.set_editor_property('parameter_name', 'Color')
color.set_editor_property('default_value', unreal.LinearColor(.6, .65, .6, 1))
matlib.connect_material_property(color, '', unreal.MaterialProperty.MP_BASE_COLOR)
fill = matlib.create_material_expression(surface, unreal.MaterialExpressionMultiply, -120, 240)
fill.set_editor_property('const_b', .24)
matlib.connect_material_expressions(color, '', fill, 'A')
matlib.connect_material_property(fill, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
rough = matlib.create_material_expression(surface, unreal.MaterialExpressionConstant, -350, 160)
rough.set_editor_property('r', .85)
matlib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
matlib.recompile_material(surface)
lib.save_loaded_asset(surface)

cloth = material('M_Cloth')
matlib.delete_all_material_expressions(cloth)
cloth.set_editor_property('two_sided', True)
tex = matlib.create_material_expression(cloth, unreal.MaterialExpressionTextureSampleParameter2D, -350, 0)
tex.set_editor_property('parameter_name', 'Pattern')
tex.set_editor_property('texture', lib.load_asset('/Game/Indigo/Textures/T_AA_0'))
matlib.connect_material_property(tex, 'RGB', unreal.MaterialProperty.MP_BASE_COLOR)
rough = matlib.create_material_expression(cloth, unreal.MaterialExpressionConstant, -350, 240)
rough.set_editor_property('r', .95)
matlib.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
matlib.recompile_material(cloth)
lib.save_loaded_asset(cloth)

bp_path = '/Game/Indigo/Blueprints/BP_IndigoWorkbench'
bp = lib.load_asset(bp_path) if lib.does_asset_exist(bp_path) else None
if not bp:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.load_class(None, '/Script/IndigoWhitebox.IndigoWorkbench'))
    bp = assets.create_asset('BP_IndigoWorkbench', '/Game/Indigo/Blueprints', unreal.Blueprint, factory)
    lib.save_loaded_asset(bp)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path = '/Game/Indigo/Maps/L_DyeWorkshop'
if lib.does_asset_exist(map_path):
    levels.load_level(map_path)
else:
    if not levels.new_level(map_path):
        raise RuntimeError('Cannot create map: ' + map_path)
    bench = actors.spawn_actor_from_class(lib.load_blueprint_class(bp_path), unreal.Vector(0, 0, 0))
    bench.set_actor_label('Indigo Workbench - editable native whitebox')
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 700), unreal.Rotator(-55, -35, 0))
    sun.set_actor_label('Workshop Sun')
    sun.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('intensity', 4.0)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property('intensity', 1.2)
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(640, 900, 880), unreal.Rotator(-36, -125, 0))
levels.save_current_level()
lib.save_directory('/Game/Indigo', only_if_is_dirty=False, recursive=True)
unreal.log('INDIGO_ASSETS_COMPLETE: 52 textures, 2 materials, native Blueprint and map')
