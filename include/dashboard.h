#pragma once

#include <Arduino.h>

// Self-contained dashboard stored in flash. No internet assets are required.
const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
  <meta name="theme-color" content="#090b10">
  <title>RGB Studio</title>
  <style>
    :root{color-scheme:dark;--bg:#080a0f;--panel:#11141d;--panel2:#181c27;--line:#292e3b;--text:#f5f7fb;--muted:#8e96a8;--accent:#806cff}
    *{box-sizing:border-box}body{margin:0;min-height:100vh;background:radial-gradient(circle at 50% -10%,#292342 0,transparent 38%),var(--bg);color:var(--text);font:15px/1.45 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;padding:22px}
    main{width:min(100%,760px);margin:auto}button,input{font:inherit}header{display:flex;align-items:center;justify-content:space-between;margin:8px 0 18px}h1{font-size:clamp(1.6rem,5vw,2.25rem);letter-spacing:-.045em;margin:0}.status{display:flex;align-items:center;gap:8px;color:var(--muted);font-size:.78rem}.dot{width:8px;height:8px;border-radius:50%;background:#38dc8b;box-shadow:0 0 12px #38dc8b}
    .card{background:linear-gradient(145deg,#171a24ee,#10121aee);border:1px solid var(--line);border-radius:22px;padding:clamp(17px,4vw,26px);box-shadow:0 24px 70px #0007}.segments{display:grid;grid-template-columns:1fr 1fr;gap:12px}.segment{position:relative;display:grid;grid-template-columns:64px 1fr;align-items:center;gap:14px;text-align:left;color:var(--text);background:var(--panel2);border:1px solid var(--line);border-radius:17px;padding:14px;cursor:pointer;transition:.15s}.segment:hover,.segment:focus-visible{border-color:#696f81;outline:none}.segment.active{border-color:#a99cff;box-shadow:0 0 0 2px #806cff33}.orb{width:64px;aspect-ratio:1;border-radius:50%;background:var(--color);box-shadow:0 0 20px color-mix(in srgb,var(--color) 65%,transparent),inset 0 0 14px #fff4}.segment strong{display:block;font-size:.98rem}.segment small{color:var(--muted)}
    .linkrow{display:flex;align-items:center;justify-content:space-between;margin:16px 2px 25px}.linkcopy strong{display:block}.linkcopy small{color:var(--muted)}.switch{position:relative;width:52px;height:30px}.switch input{opacity:0;width:0;height:0}.switch span{position:absolute;inset:0;border-radius:20px;background:#353a49;cursor:pointer;transition:.2s}.switch span:before{content:"";position:absolute;width:22px;height:22px;left:4px;top:4px;border-radius:50%;background:white;transition:.2s}.switch input:checked+span{background:var(--accent)}.switch input:checked+span:before{translate:22px 0}
    .editor{border-top:1px solid var(--line);padding-top:23px}.editorhead{display:grid;grid-template-columns:1fr 120px;gap:16px;align-items:end;margin-bottom:21px}.eyebrow{margin:0 0 4px;color:#a99cff;font-size:.72rem;text-transform:uppercase;letter-spacing:.13em;font-weight:800}.hex{font-size:1.35rem;font-weight:750;margin:0}input[type=color]{width:100%;height:46px;border:1px solid var(--line);border-radius:12px;padding:5px;background:#0b0d12;cursor:pointer}.control{margin:16px 0}.label{display:flex;justify-content:space-between;margin-bottom:8px;color:var(--muted);font-size:.86rem}.value{color:var(--text);font-variant-numeric:tabular-nums}input[type=range]{width:100%;height:7px;margin:0;appearance:none;border-radius:10px;background:linear-gradient(90deg,var(--track) 0 var(--fill),#303543 var(--fill));outline:none}input[type=range]::-webkit-slider-thumb{appearance:none;width:21px;height:21px;border:3px solid white;border-radius:50%;background:var(--thumb);box-shadow:0 2px 8px #0008}input[type=range]::-moz-range-thumb{width:15px;height:15px;border:3px solid white;border-radius:50%;background:var(--thumb)}
    .presets{display:grid;grid-template-columns:repeat(7,1fr);gap:9px;margin:23px 0}.swatch{aspect-ratio:1;border:1px solid #ffffff22;border-radius:13px;background:var(--swatch);cursor:pointer;box-shadow:inset 0 1px #fff3;transition:.12s}.swatch:hover,.swatch:focus-visible{translate:0 -2px;border-color:white;outline:none}.off{background:#222633;position:relative}.off:after{content:"";position:absolute;width:55%;height:2px;background:#ff5571;rotate:-45deg;inset:50% auto auto 23%}
    .effects{margin-top:25px;padding-top:22px;border-top:1px solid var(--line)}.sectiontitle{display:flex;align-items:end;justify-content:space-between;margin-bottom:12px}.sectiontitle strong{font-size:.95rem}.sectiontitle small{color:var(--muted)}.effectgrid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.effect{border:1px solid var(--line);border-radius:11px;background:#1a1e29;color:var(--muted);padding:11px 6px;cursor:pointer}.effect.active{color:white;background:#30284d;border-color:#8d78ff}.speed{margin:17px 0 0}.speed.hidden{display:none}.meta{display:flex;justify-content:space-between;gap:12px;color:var(--muted);font-size:.73rem;margin:14px 4px 0}.saved{color:#57dc9a}
    @media(max-width:560px){body{padding:14px}.segment{grid-template-columns:46px 1fr;padding:11px;gap:10px}.orb{width:46px}.segment small{font-size:.66rem}.editorhead{grid-template-columns:1fr 100px}.effectgrid{grid-template-columns:1fr 1fr}.presets{gap:6px}.card{border-radius:19px}}
  </style>
</head>
<body>
  <main>
    <header><h1>RGB Studio</h1><div class="status"><span class="dot"></span><span id="connection">Connected</span></div></header>
    <section class="card">
      <div class="segments" aria-label="Select a segment">
        <button class="segment active" data-segment="0"><span class="orb" id="orb0"></span><span><strong>Segment 1</strong><small>R4 · G5 · B6</small></span></button>
        <button class="segment" data-segment="1"><span class="orb" id="orb1"></span><span><strong>Segment 2</strong><small>R7 · G8 · B9</small></span></button>
      </div>
      <div class="linkrow">
        <div class="linkcopy"><strong>Link segments</strong><small>Changes apply to both zones</small></div>
        <label class="switch"><input id="linked" type="checkbox" checked aria-label="Link both segments"><span></span></label>
      </div>
      <div class="editor">
        <div class="editorhead"><div><p class="eyebrow" id="editing">Editing linked segments</p><p class="hex" id="hex">#7C5CFF</p></div><input id="picker" type="color" aria-label="Choose color"></div>
        <div id="sliders"></div>
        <div class="presets" aria-label="Color presets">
          <button class="swatch" style="--swatch:#ff3b5c" data-color="255,59,92" aria-label="Red"></button><button class="swatch" style="--swatch:#ff982e" data-color="255,152,46" aria-label="Orange"></button><button class="swatch" style="--swatch:#ffe05c" data-color="255,224,92" aria-label="Yellow"></button><button class="swatch" style="--swatch:#4fe28b" data-color="79,226,139" aria-label="Green"></button><button class="swatch" style="--swatch:#35b9ff" data-color="53,185,255" aria-label="Blue"></button><button class="swatch" style="--swatch:#9d72ff" data-color="157,114,255" aria-label="Purple"></button><button class="swatch off" data-color="0,0,0" aria-label="Off"></button>
        </div>
      </div>
      <div class="effects">
        <div class="sectiontitle"><strong>Motion</strong><small>Runs directly on the ESP32</small></div>
        <div class="effectgrid"><button class="effect active" data-effect="solid">Solid</button><button class="effect" data-effect="pulse">Pulse</button><button class="effect" data-effect="cycle">Color cycle</button><button class="effect" data-effect="alternate">Alternate</button></div>
        <div class="control speed hidden" id="speedWrap"><div class="label"><label for="speed">Effect speed</label><span class="value" id="speedValue">128</span></div><input id="speed" type="range" min="0" max="255" value="128" style="--track:#9d72ff;--thumb:#9d72ff;--fill:50%"></div>
      </div>
      <div class="linkrow" style="margin:24px 2px 0;padding-top:22px;border-top:1px solid var(--line)">
        <div class="linkcopy"><strong>Enable serial debugging</strong><small>Stream live GPIO and system telemetry</small></div>
        <label class="switch"><input id="debugEnabled" type="checkbox" checked aria-label="Enable serial debugging"><span></span></label>
      </div>
    </section>
    <div class="meta"><span>ESP32-S3 · 192.168.4.1</span><span class="saved" id="saved">Settings saved on device</span></div>
  </main>
  <script>
    const state={linked:true,effect:'solid',speed:128,debug_enabled:true,segments:[{r:124,g:92,b:255,brightness:180},{r:124,g:92,b:255,brightness:180}]};
    let active=0,manualActive=0,timer,revision=0,deviceOffset=0;
    const defs=[['r','Red','#ff4d65'],['g','Green','#45dc87'],['b','Blue','#4aa8ff'],['brightness','Brightness','#fff']];
    const $=s=>document.querySelector(s), sliders=$('#sliders'), picker=$('#picker'), linked=$('#linked');
    defs.forEach(([key,label,color])=>sliders.insertAdjacentHTML('beforeend',`<div class="control"><div class="label"><label for="${key}">${label}</label><span class="value" id="${key}Value">0</span></div><input id="${key}" type="range" min="0" max="255" style="--track:${color};--thumb:${color};--fill:0%"></div>`));
    const toHex=n=>Number(n).toString(16).padStart(2,'0');
    const cssColor=s=>{const k=s.brightness/255;return `rgb(${s.r*k},${s.g*k},${s.b*k})`};
    function render(){
      const s=state.segments[active];
      defs.forEach(([key])=>{const e=$('#'+key);e.value=s[key];e.style.setProperty('--fill',s[key]/2.55+'%');$('#'+key+'Value').textContent=s[key]});
      const color='#'+toHex(s.r)+toHex(s.g)+toHex(s.b);picker.value=color;$('#hex').textContent=color.toUpperCase();linked.checked=state.linked;
      state.segments.forEach((x,i)=>$('#orb'+i).style.setProperty('--color',cssColor(x)));
      document.querySelectorAll('.segment').forEach((e,i)=>e.classList.toggle('active',i===active));
      $('#editing').textContent=state.linked?'Editing linked segments':'Editing Segment '+(active+1);
      document.querySelectorAll('.effect').forEach(e=>e.classList.toggle('active',e.dataset.effect===state.effect));
      $('#speed').value=state.speed;$('#speed').style.setProperty('--fill',state.speed/2.55+'%');$('#speedValue').textContent=state.speed;$('#speedWrap').classList.toggle('hidden',state.effect==='solid');
      $('#debugEnabled').checked=state.debug_enabled;
    }
    function body(){const p=new URLSearchParams({linked:state.linked?'1':'0',effect:state.effect,speed:state.speed,active_segment:active,debug_enabled:state.debug_enabled?'1':'0'});state.segments.forEach((s,i)=>{const n=i+1;p.set('s'+n+'r',s.r);p.set('s'+n+'g',s.g);p.set('s'+n+'b',s.b);p.set('s'+n+'brightness',s.brightness)});return p}
    function applyServerState(data,initial=false){deviceOffset=data.uptime_ms-performance.now();const {active_segment,uptime_ms,...fresh}=data;Object.assign(state,fresh);if(initial&&state.effect!=='alternate'){active=active_segment;manualActive=active_segment}}
    function send(){
      clearTimeout(timer);const request=++revision,payload=body();$('#saved').textContent='Saving…';
      timer=setTimeout(async()=>{try{const response=await fetch('/api/state',{method:'POST',body:payload});const data=await response.json();if(request!==revision)return;applyServerState(data);$('#connection').textContent='Connected';$('#saved').textContent='Saved on device';render()}catch(e){if(request!==revision)return;$('#connection').textContent='Reconnecting…';$('#saved').textContent='Not saved'}},55)
    }
    function changeColor(rgb){const s=state.segments[active];[s.r,s.g,s.b]=rgb;if(state.linked)state.segments[1-active]={...s};render();send()}
    document.querySelectorAll('.segment').forEach(e=>e.addEventListener('click',()=>{active=+e.dataset.segment;manualActive=active;render();send()}));
    linked.addEventListener('change',()=>{state.linked=linked.checked;if(state.linked)state.segments[1-active]={...state.segments[active]};render();send()});
    defs.forEach(([key])=>$('#'+key).addEventListener('input',e=>{state.segments[active][key]=+e.target.value;if(state.linked)state.segments[1-active]={...state.segments[active]};render();send()}));
    picker.addEventListener('input',e=>changeColor([1,3,5].map(i=>parseInt(e.target.value.slice(i,i+2),16))));
    document.querySelectorAll('[data-color]').forEach(e=>e.addEventListener('click',()=>changeColor(e.dataset.color.split(',').map(Number))));
    document.querySelectorAll('.effect').forEach(e=>e.addEventListener('click',()=>{const wasAlternate=state.effect==='alternate';state.effect=e.dataset.effect;if(wasAlternate&&state.effect!=='alternate')active=manualActive;render();send()}));
    $('#speed').addEventListener('input',e=>{state.speed=+e.target.value;render();send()});
    $('#debugEnabled').addEventListener('change',e=>{state.debug_enabled=e.target.checked;render();send()});
    const initialRevision=revision;
    fetch('/api/state').then(r=>r.json()).then(data=>{if(revision!==initialRevision)return;applyServerState(data,true);render()}).catch(()=>$('#connection').textContent='Reconnecting…');
    setInterval(()=>{if(state.effect!=='alternate')return;const period=6000-(state.speed/255)*5500;const next=((performance.now()+deviceOffset)%period)<period/2?0:1;if(next!==active){active=next;render()}},40);render();
  </script>
</body>
</html>
)HTML";
