export function frameAt(tick, ticks, count) {
  if (tick < 0 || ticks < 1 || count < 1) return null;
  const frame = Math.floor(tick / ticks);
  return frame < count ? frame : null;
}
export function latestCue(timeline, tick) {
  if (!timeline || tick < 0 || tick >= timeline.releaseTick + timeline.recoveryTicks) return null;
  let event = null;
  for (const cue of timeline.events) if (cue.tick <= tick) event = cue;
  return event;
}
export function simulate(def, tick, targetCount = 1) {
  const origin = {x:def.kind === 'burst'?150:40,y:110};
  const targets = Array.from({length:targetCount}, (_,i)=>({x:150+i*24,y:110}));
  const attempts = new Map(), direct = new Set(), impacts = [], trails = [];
  let x = origin.x, alive = tick >= 0;
  function contact(index, t) {
    const old = attempts.get(index);
    if ((!old && attempts.size >= def.maxTargets) || (old && (old.count >= def.maxRepeats || t-old.tick < def.repeatTicks))) return false;
    attempts.set(index, {count:(old?.count||0)+1,tick:t});
    impacts.push({...targets[index],tick:t});
    return true;
  }
  for(let t=0;t<=Math.min(tick,def.lifetime-1) && alive;t++) {
    if(def.kind === 'burst') {
      for(let i=0;i<targets.length;i++) if(t%def.repeatTicks===0 && Math.abs(targets[i].x-origin.x)<=def.radius) contact(i,t);
    } else {
      for(let step=0;step<def.speed && alive;step++) {
        x++;
        for(let i=0;i<targets.length;i++) if(Math.abs(targets[i].x-x)<=def.radius && contact(i,t)) {
          direct.add(i);
          if(def.splashRadius) for(let j=0;j<targets.length;j++) if(Math.abs(targets[j].x-targets[i].x)<=def.splashRadius) contact(j,t);
          if(direct.size>=def.pierceTargets) {alive=false;break;}
        }
      }
    }
    if(alive && def.trailEffect && t%def.trailTicks===0) trails.push({x,y:110,tick:t});
  }
  if(tick>=def.lifetime-1) alive=false;
  return {x,y:110,alive,targets,impacts,trails,contacts:impacts.length};
}
