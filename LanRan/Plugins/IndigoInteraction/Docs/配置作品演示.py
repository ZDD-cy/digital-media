import unreal,json,traceback

r={}
try:
    assert not unreal.EditorLevelLibrary.get_game_world()
    actors=list(unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors())
    rig=next(a for a in actors if a.get_actor_label()=='Indigo_CraftPreview')
    mat=unreal.load_asset('/Game/Core/IndigoPreview/M_FinalCloth')
    if not mat:
        mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_FinalCloth','/Game/Core/IndigoPreview',unreal.Material,unreal.MaterialFactoryNew())
    if mat:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
        mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
        lib=unreal.MaterialEditingLibrary
        uv=lib.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate)
        color=lib.create_material_expression(mat,unreal.MaterialExpressionVectorParameter)
        color.set_editor_property('parameter_name','DyeColor');color.set_editor_property('default_value',unreal.LinearColor(.8,.85,.76,1))
        expressions={'UV':uv,'DyeColor':color}
        for name,default in [('PatternLeft',1),('PatternRight',2),('ShowPattern',0)]:
            expr=lib.create_material_expression(mat,unreal.MaterialExpressionScalarParameter)
            expr.set_editor_property('parameter_name',name);expr.set_editor_property('default_value',float(default));expressions[name]=expr
        custom=lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
        custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs=[]
        for name in expressions:
            item=unreal.CustomInput();item.set_editor_property('input_name',name);inputs.append(item)
        custom.set_editor_property('inputs',inputs)
        custom.set_editor_property('code','''
float2 p=UV*float2(12,8);
float2 q=abs(frac(p)-0.5);
float id=UV.x>0.5?PatternLeft:PatternRight;
float a=1-smoothstep(0.035,0.075,abs(q.x+q.y-0.36));
float b=1-smoothstep(0.035,0.075,min(q.x,q.y));
float c=1-smoothstep(0.035,0.075,abs(length(q)-0.28));
float mask=id<1.5?a:(id<2.5?b:c);
float border=1-smoothstep(0.015,0.028,min(min(UV.x,1-UV.x),min(UV.y,1-UV.y)));
float seam=1-smoothstep(0.006,0.012,abs(UV.x-0.5));
return lerp(DyeColor,float3(0.85,0.88,0.8),saturate(max(mask,max(border,seam))*ShowPattern));
''')
        for name,expr in expressions.items():assert lib.connect_material_expressions(expr,'',custom,name)
        assert lib.connect_material_property(custom,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        lib.recompile_material(mat)
        assert unreal.EditorAssetLibrary.save_loaded_asset(mat,False)
    with unreal.ScopedEditorTransaction('Connect wash and final cloth preview'):
        for name in ['cloth','moving_cloth']:rig.get_editor_property(name).set_material(0,mat)
        rig.get_editor_property('wash_basin').set_material(0,unreal.load_asset('/Game/Core/IndigoPreview/M_PreviewWood'))
        rig.get_editor_property('wash_water').set_material(0,unreal.load_asset('/Game/Core/IndigoPreview/M_PreviewFoldLine'))
    assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    r={'success':True,'material':mat.get_path_name()}
except Exception:r={'success':False,'error':traceback.format_exc()}
unreal.log(json.dumps(r,ensure_ascii=False,indent=2))
