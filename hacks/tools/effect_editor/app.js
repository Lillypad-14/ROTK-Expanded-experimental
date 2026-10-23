import {frameAt, latestCue, simulate} from '/model.mjs';
const $ = id => document.getElementById(id);
const clone = value => structuredClone(value);
let project, metadata, baseline, images = new Map(), mode='assets', selected=0, tick=0, playing=false, history=[];
let revision=0, busy=false;
const token=document.querySelector('meta[name=editor-token]').content;
const ctx=$('stage').getContext('2d');
const colors={white:'#eaf0e5',ember:'#fca267',green:'#97c57d',blue:'#81b9ef',nazgul:'#c79de1',night:'#9698bd',gold:'#e6c575'};
function entries(){return mode==='assets'||mode==='effects'?project.fx[mode]:project[mode][mode];}
function current(){return entries()[selected];}
function status(text,error=false){$('status').textContent=text;$('status').className=error?'error':'';}
function save(){history.push(clone(project));if(history.length>50)history.shift();revision++;}
function changed(){revision++;if($('entry').options[selected])$('entry').options[selected].textContent=current().name??current().key;$('json').value=JSON.stringify(current(),null,2);status('Draft changed. Validate before export.');render();}
function options(values){return values.map(value=>[value,value]);}
function field(parent,label,value,update,{type='number',values=null,min=0,max=3600,wide=false}={}){
 const wrap=document.createElement('label');wrap.textContent=label;if(wide)wrap.className='wide';
 const input=document.createElement(values?'select':'input');
 if(values)for(const [key,text] of values){const option=document.createElement('option');option.value=key;option.textContent=text;input.append(option);}
 else {input.type=type;input.min=min;input.max=max;input.step=1;}
 if(type==='checkbox')input.checked=Boolean(value);else input.value=value??'';
 input.addEventListener('change',()=>{save();update(type==='checkbox'?input.checked:type==='number'&&!values?Number(input.value):input.value);changed();});
 wrap.append(input);parent.append(wrap);return input;
}
function effectChoices(nullable=true){return [...(nullable?[['','None']]:[]),...project.fx.effects.map(e=>[e.key,e.name])];}
function showFields(){
 const el=$('fields'),item=current();el.replaceChildren();
 field(el,'Key',item.key,v=>item.key=v,{type:'text',wide:true});
 if(item.name!==undefined)field(el,'Display name',item.name,v=>item.name=v,{type:'text',wide:true});
 if(mode==='assets'){
  field(el,'Frame ticks',item.frameTicks??2,v=>item.frameTicks=v,{min:1,max:255});
  const anchor=item.anchor??[Math.floor(item.width/2),item.height-1];
  for(let i=0;i<2;i++)field(el,i?'Anchor Y':'Anchor X',anchor[i],v=>{item.anchor=[...anchor];item.anchor[i]=v;anchor[i]=v;},{min:-128,max:127});
  field(el,'Palette ID',item.id,v=>item.id=v,{max:31});
 }else if(mode==='effects'){
  item.layers.forEach((layer,index)=>{
   const card=document.createElement('div');card.className='layer';const heading=document.createElement('h3');heading.textContent=`Layer ${index+1}`;card.append(heading);el.append(card);
   field(card,'Source',layer.asset?'asset':'shape',v=>{delete layer.asset;delete layer.shape;delete layer.color;delete layer.duration;if(v==='asset')layer.asset=project.fx.assets[0].key;else{layer.shape=metadata.shapes[0];layer.color='white';}showFields();},{values:options(project.fx.assets.length?['asset','shape']:['shape']),wide:true});
   if(layer.asset)field(card,'Sprite',layer.asset,v=>layer.asset=v,{values:options(project.fx.assets.map(a=>a.key)),wide:true});
   else{
    field(card,'Shape',layer.shape,v=>layer.shape=v,{values:options(metadata.shapes),wide:true});
    field(card,'Color',layer.color,v=>layer.color=v,{values:options(metadata.colors)});
    field(card,'Duration',layer.duration??0,v=>{if(v)layer.duration=v;else delete layer.duration;},{max:65535});
   }
   field(card,'Frame ticks',layer.frameTicks??(project.fx.assets.find(a=>a.key===layer.asset)?.frameTicks??2),v=>layer.frameTicks=v,{min:1,max:255});
   field(card,'Layer',layer.layer??'front',v=>layer.layer=v,{values:options(['front','behind'])});
   field(card,'Attachment',layer.attachment??'follow',v=>layer.attachment=v,{values:options(['follow','fixed'])});
   const remove=document.createElement('button');remove.textContent='Remove layer';remove.disabled=item.layers.length===1;remove.onclick=()=>{save();item.layers.splice(index,1);showFields();changed();};card.append(remove);
  });
  const add=document.createElement('button');add.textContent='Add layer';add.className='wide';add.disabled=item.layers.length>=4;add.onclick=()=>{save();item.layers.push({shape:'spark',color:'white'});showFields();changed();};el.append(add);
 }else if(mode==='timelines'){
  field(el,'Release tick',item.releaseTick,v=>item.releaseTick=v);
  field(el,'Recovery ticks',item.recoveryTicks,v=>item.recoveryTicks=v);
  item.events.forEach((event,index)=>{
   const card=document.createElement('div');card.className='layer';el.append(card);
   field(card,`Cue ${index+1} tick`,event.tick,v=>{event.tick=v;item.events.sort((a,b)=>a.tick-b.tick);showFields();});
   field(card,'Anchor',event.anchor,v=>event.anchor=v,{values:options(['owner','target','position'])});
   field(card,'Effect',event.effect,v=>event.effect=v,{values:effectChoices(false),wide:true});
   const remove=document.createElement('button');remove.textContent='Remove cue';remove.onclick=()=>{save();item.events.splice(index,1);showFields();changed();};card.append(remove);
  });
  const add=document.createElement('button');add.textContent='Add visual cue';add.className='wide';add.disabled=item.events.length>=16;add.onclick=()=>{save();item.events.push({tick:item.releaseTick,effect:project.fx.effects[0].key,anchor:'owner'});item.events.sort((a,b)=>a.tick-b.tick);showFields();changed();};el.append(add);
 }else{
  field(el,'Kind',item.kind,v=>{item.kind=v;if(v==='straight'&&!item.speed)item.speed=6;showFields();},{values:options(['straight','burst']),wide:true});
  for(const key of ['speed','radius','lifetime','splashRadius','pierceTargets','maxTargets','maxRepeats','repeatTicks','stunTicks','knockback','trailTicks'])field(el,key.replace(/[A-Z]/g,c=>' '+c.toLowerCase()),item[key],v=>item[key]=v,{min:metadata.limits[key][0],max:metadata.limits[key][1]});
  field(el,'Stop at walls',item.stopOnWalls,v=>item.stopOnWalls=v,{type:'checkbox'});
  for(const key of ['travelEffect','trailEffect','impactEffect'])field(el,key.replace('Effect',' effect'),item[key]??'',v=>item[key]=v||null,{values:effectChoices(),wide:true});
  for(const key of ['launchSound','impactSound'])field(el,key,item[key],v=>item[key]=v,{values:options(metadata.sounds),wide:true});
  item.damage.forEach((damage,index)=>field(el,`Level ${index+1} damage`,damage,v=>item.damage[index]=v,{max:65535}));
 }
 $('json').value=JSON.stringify(item,null,2);
}
function refresh(reset=true){
 if(!entries().length){mode=['effects','timelines','deliveries','assets'].find(key=>key==='assets'||key==='effects'?project.fx[key].length:project[key][key].length);selected=0;}
 const list=$('entry');list.replaceChildren();entries().forEach((item,index)=>{const option=document.createElement('option');option.value=index;option.textContent=item.name??item.key;list.append(option);});
 selected=Math.min(selected,entries().length-1);list.value=selected;
 const cast=$('cast'),previous=cast.value;cast.replaceChildren();[['','No windup'],...project.timelines.timelines.map((t,i)=>[String(i),t.name])].forEach(([value,text])=>{const option=document.createElement('option');option.value=value;option.textContent=text;cast.append(option);});cast.value=previous||'';
 document.querySelectorAll('nav button').forEach(b=>b.classList.toggle('active',b.dataset.mode===mode));
 if(reset){tick=0;playing=false;$('play').textContent='Play';}
 showFields();render();
}
function sprite(spec,layer,x,y,age,hold=false){
 const cached=metadata.assets[spec.key]??Object.values(metadata.assets).find(a=>a.sheet===spec.sheet);
 const image=images.get(spec.key)??images.get(spec.sheet);if(!cached||!image)return;
 const ticks=layer.frameTicks??spec.frameTicks??2;
 const frame=hold?Math.min(cached.frames-1,Math.floor(age/ticks)):frameAt(age,ticks,cached.frames);
 if(frame===null)return;
 const anchor=spec.anchor??[Math.floor(spec.width/2),spec.height-1];
 ctx.drawImage(image,frame*spec.width,0,spec.width,spec.height,x-anchor[0],y-anchor[1],spec.width,spec.height);
 if($('guides').checked){ctx.strokeStyle='#b7d7bb';ctx.lineWidth=.5;ctx.strokeRect(x-anchor[0],y-anchor[1],spec.width,spec.height);}
}
function effect(key,x,y,age){
 if(age<0)return;const recipe=project.fx.effects.find(e=>e.key===key);if(!recipe)return;
 const layers=[...recipe.layers].sort((a,b)=>(a.layer==='behind'?-1:1)-(b.layer==='behind'?-1:1));
 for(const layer of layers){
  if(layer.asset){const spec=project.fx.assets.find(a=>a.key===layer.asset);if(spec)sprite(spec,layer,x,y,age);}
  else if(age<(layer.duration??24)){
   ctx.strokeStyle=colors[layer.color]??'#ddd';ctx.fillStyle=ctx.strokeStyle;ctx.lineWidth=1;
   const radius=5+(age%24)/2;ctx.beginPath();ctx.arc(x,y-5,radius,0,Math.PI*2);ctx.stroke();ctx.font='6px monospace';ctx.textAlign='center';ctx.fillText(layer.shape?.replaceAll('_',' ')??'stock FX',x,y-20);
  }
 }
}
function cross(x,y,color){ctx.strokeStyle=color;ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(x-3,y);ctx.lineTo(x+3,y);ctx.moveTo(x,y-3);ctx.lineTo(x,y+3);ctx.stroke();}
function actor(x,y,label){ctx.fillStyle='#637d6c';ctx.fillRect(x-3,y-13,6,12);ctx.fillStyle='#cbb88b';ctx.fillRect(x-2,y-17,4,4);ctx.font='6px monospace';ctx.textAlign='center';ctx.fillStyle='#b6c8bd';ctx.fillText(label,x,y+10);}
function timelineView(timeline,max){
 $('timeline').replaceChildren();
 function mark(cls,position,title,width){const el=document.createElement('span');el.className=cls;el.style.left=`${100*position/max}%`;if(width!==undefined)el.style.width=`${width}%`;el.title=title;$('timeline').append(el);}
 if(timeline){mark('recovery',timeline.releaseTick,'Recovery',100*timeline.recoveryTicks/max);mark('release',timeline.releaseTick,'Release');for(const cue of timeline.events)mark('cue',cue.tick,`${cue.tick}: ${cue.effect} / ${cue.anchor}`);}
 mark('needle',tick,'Current tick');
}
function render(){
 if(!project)return;const item=current();if(!item)return;
 ctx.imageSmoothingEnabled=false;
 ctx.fillStyle='#182520';ctx.fillRect(0,0,240,160);
 if($('grid').checked){ctx.strokeStyle='#25362b';ctx.lineWidth=.3;for(let x=0;x<240;x+=8){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,160);ctx.stroke();}for(let y=0;y<160;y+=8){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(240,y);ctx.stroke();}}
 let max=120, timeline=null,metrics='';$('title').textContent=item.name??item.key;
 if(mode==='assets'){
  const data=metadata.assets[item.key]??Object.values(metadata.assets).find(a=>a.sheet===item.sheet);max=Math.max(0,(data?.frames??1)*(item.frameTicks??2)-1);
  sprite(item,{},120,110,tick,true);if($('guides').checked)cross(120,110,'#ffd587');
  metrics=data?`${item.width}x${item.height} | frame ${Math.min(data.frames,Math.floor(tick/(item.frameTicks??2))+1)}/${data.frames} | ${data.colors}/15 colors | ${data.tileBytes}/2304 tile bytes`:'Validate to load this sprite';
 }else if(mode==='effects'){
  actor(120,110,'CASTER');effect(item.key,120,110,tick);
  metrics=`${item.layers.length}/4 layers | stock shapes are schematic`;
 }else if(mode==='timelines'){
  timeline=item;max=Math.max(1,item.releaseTick+item.recoveryTicks);actor(40,110,'CASTER');actor(150,110,'TARGET');
  const cue=latestCue(item,tick);if(cue)effect(cue.effect,cue.anchor==='target'?150:40,110,tick-cue.tick);
  metrics=`Release ${item.releaseTick}t | recovery ${item.recoveryTicks}t | ${(max/30).toFixed(2)}s total`;
 }else{
  timeline=$('cast').value===''?null:project.timelines.timelines[Number($('cast').value)];
  const release=timeline?.releaseTick??0;max=Math.max(1,release+item.lifetime+60,timeline?timeline.releaseTick+timeline.recoveryTicks:0);
  actor(40,110,'CASTER');
  const result=simulate(item,tick-release,Number($('targets').value));
  for(const target of result.targets)actor(target.x,target.y,'TARGET');
  if(timeline){const cue=latestCue(timeline,tick);if(cue)effect(cue.effect,cue.anchor==='target'?150:40,110,tick-cue.tick);}
  for(const trail of result.trails)effect(item.trailEffect,trail.x,trail.y,tick-release-trail.tick);
  if(result.alive&&item.travelEffect){
   const recipe=project.fx.effects.find(e=>e.key===item.travelEffect);
   const duration=Math.max(1,...(recipe?.layers??[]).map(l=>l.asset?(metadata.assets[l.asset]?.frames??1)*(l.frameTicks??project.fx.assets.find(a=>a.key===l.asset)?.frameTicks??2):(l.duration??24)));
   effect(item.travelEffect,result.x,result.y,(tick-release)%duration);
  }
  for(const impact of result.impacts)effect(item.impactEffect,impact.x,impact.y,tick-release-impact.tick);
  if($('guides').checked&&result.alive){ctx.strokeStyle='#98d1f0';ctx.beginPath();ctx.arc(result.x,result.y,item.radius,0,Math.PI*2);ctx.stroke();cross(result.x,result.y,'#ffd587');}
  metrics=`${item.kind} | ${item.speed}px/tick | ${result.contacts} contact attempts | bursts centered on first target`;
 }
 if($('guides').checked&&mode!=='assets'&&mode!=='deliveries')cross(mode==='effects'?120:40,110,'#ffd587');
 $('metrics').textContent=metrics;$('scrub').max=max;tick=Math.min(tick,max);$('scrub').value=tick;$('tick').textContent=`Tick ${tick} | ${(tick/30).toFixed(2)}s`;timelineView(timeline,max||1);
}
async function loadImages(){
 images.clear();await Promise.all(Object.entries(metadata.assets).map(async([key,data])=>{const image=new Image();image.src=data.image;await image.decode();images.set(key,image);const spec=project.fx.assets.find(a=>a.key===key);if(spec)images.set(spec.sheet,image);}));
}
async function reload(){
 try{const response=await fetch('/api/catalog');const data=await response.json();if(!response.ok)throw Error(data.error);metadata=data;project=clone(data.project);baseline=clone(project);history=[];revision++;selected=0;await loadImages();refresh();status('Source loaded. Edit a draft, then validate or export.');}
 catch(error){status(error.message,true);}
}
async function validate(exporting=false){
 if(busy)return;busy=true;const snapshot=JSON.stringify(project),version=revision;
 $('validate').disabled=$('export').disabled=true;status('Running ROM content validators...');
 try{
  const response=await fetch(exporting?'/api/export':'/api/validate',{method:'POST',headers:{'Content-Type':'application/json','X-Editor-Token':token},body:snapshot});
  if(!response.ok){let data;try{data=await response.json();}catch{throw Error('Validation request failed');}throw Error(data.error);}
  if(exporting){const url=URL.createObjectURL(await response.blob());const a=document.createElement('a');a.href=url;a.download='rotk-effects-draft.zip';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);status('Validated bundle downloaded. Review its hacks/content files before applying.');}
  else{const data=await response.json();if(version===revision&&data.assets){metadata.assets={...metadata.assets,...data.assets};await loadImages();render();}status(version===revision?'Valid: sprite limits, effect references, timelines and deliveries passed.':'The submitted snapshot passed; newer edits still need validation.');}
 }catch(error){status(error.message,true);}finally{busy=false;$('validate').disabled=$('export').disabled=false;}
}
$('stage').addEventListener('click',event=>{
 if(mode!=='assets'||!project)return;
 const spec=current(),anchor=spec.anchor??[Math.floor(spec.width/2),spec.height-1];
 const rect=$('stage').getBoundingClientRect();
 const x=Math.floor((event.clientX-rect.left)*240/rect.width-(120-anchor[0]));
 const y=Math.floor((event.clientY-rect.top)*160/rect.height-(110-anchor[1]));
 if(x<0||y<0||x>=spec.width||y>=spec.height)return;
 save();spec.anchor=[x,y];showFields();changed();
});
$('entry').onchange=()=>{selected=Number($('entry').value);refresh();};
for(const button of document.querySelectorAll('nav button'))button.onclick=()=>{mode=button.dataset.mode;selected=0;refresh();};
$('play').onclick=()=>{playing=!playing;$('play').textContent=playing?'Pause':'Play';};
$('scrub').oninput=()=>{playing=false;$('play').textContent='Play';tick=Number($('scrub').value);render();};
$('back').onclick=()=>{playing=false;tick=Math.max(0,tick-1);$('play').textContent='Play';render();};
$('forward').onclick=()=>{playing=false;tick=Math.min(Number($('scrub').max),tick+1);$('play').textContent='Play';render();};
$('zoom').onchange=()=>{$('stage').style.width=`${240*Number($('zoom').value)}px`;$('stage').style.height=`${160*Number($('zoom').value)}px`;};
for(const id of ['grid','guides','targets','cast'])$(id).onchange=()=>render();
$('clone').onclick=()=>{save();const item=clone(current()),used=new Set(entries().map(e=>e.key));let key=item.key+'_copy';while(used.has(key))key+='_copy';item.key=key;if(mode!=='assets')item.id=entries().length;entries().push(item);selected=entries().length-1;refresh();changed();};
$('undo').onclick=()=>{if(!history.length)return;project=history.pop();revision++;refresh(false);status('Previous draft restored.');};
async function acceptProject(draft) {
 const response=await fetch('/api/validate',{method:'POST',headers:{'Content-Type':'application/json','X-Editor-Token':token},body:JSON.stringify(draft)});
 const data=await response.json();if(!response.ok)throw Error(data.error);
 save();project=draft;metadata.assets={...metadata.assets,...data.assets};await loadImages();refresh(false);changed();status('Imported draft passed ROM validation.');
}
$('apply').onclick=async()=>{try{const item=JSON.parse($('json').value);const draft=clone(project);const list=mode==='assets'||mode==='effects'?draft.fx[mode]:draft[mode][mode];list[selected]=item;await acceptProject(draft);}catch(error){status(error.message,true);}};
$('reload').onclick=()=>{if(JSON.stringify(project)!==JSON.stringify(baseline)&&!confirm('Discard this draft and reload source files?'))return;reload();};
$('import').onchange=async()=>{try{const file=$('import').files[0];if(!file)return;const draft=JSON.parse(await file.text());await acceptProject(draft);}catch(error){status(error.message,true);}finally{$('import').value='';}};
$('validate').onclick=()=>validate();$('export').onclick=()=>validate(true);
let last=0,accumulator=0;
function animate(now){if(playing&&project){accumulator+=Math.min(250,now-last);if(accumulator>=1000/30){const steps=Math.floor(accumulator/(1000/30));accumulator%=1000/30;tick+=steps;if(tick>Number($('scrub').max)){if($('loop').checked)tick=0;else{tick=Number($('scrub').max);playing=false;$('play').textContent='Play';}}render();}}else accumulator=0;last=now;requestAnimationFrame(animate);}
window.addEventListener('beforeunload',event=>{if(project&&JSON.stringify(project)!==JSON.stringify(baseline)){event.preventDefault();event.returnValue='';}});
requestAnimationFrame(animate);reload();
