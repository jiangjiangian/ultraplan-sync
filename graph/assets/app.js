/* 《尋傘記》知識圖譜 — 互動邏輯（Cytoscape.js）。設計/行為對齊李宏毅老師頻道地圖，
   改寫為本 repo 的資料：節點=檔案/概念，邊=#include / 繼承 / 模式落點。資料來自 window.GRAPH_DATA。 */
(function () {
  "use strict";
  var DATA = window.GRAPH_DATA;
  if (!DATA) { document.getElementById("cy").innerHTML =
    "<pre style='color:#f87171;padding:20px'>找不到 data/graph-data.js，請先跑 python3 graph/build_graph.py</pre>"; return; }

  // ---- 配色（暗色主題；檔案依領域、概念依種類）----
  var DOMAIN_COL = { app:"#ed7d31", engine:"#4ade80", game:"#fbbf24", ui:"#5b9bd5",
    tests:"#a5a5a5", docs:"#c586c0", tools:"#9b8cff", resources:"#70ad47", root:"#7a8499" };
  var CONCEPT_COL = { pattern:"#ff6b9d", principle:"#2dd4bf", architecture:"#a78bfa" };
  var SHAPE = { file:"ellipse", pattern:"star", principle:"diamond", architecture:"hexagon",
    domain:"round-rectangle", bucket:"round-rectangle" };
  var DOMAIN_LABEL = { app:"app 組裝根", engine:"engine 引擎", game:"game 遊戲邏輯", ui:"ui 視圖",
    tests:"tests 測試", docs:"docs 文件", tools:"tools 工具", resources:"resources 資產", root:"root 根" };
  var KIND_ZH = { file:"程式檔", pattern:"設計模式", principle:"OO 原則", architecture:"架構元件",
    domain:"領域", bucket:"bucket" };
  var ET = {  // 邊種類 → 卡片標題（往外 / 往內）
    includes:["#include →","← 被 include"], inherits:["繼承自 →","← 子類別"],
    realizes:["落點 →","← 體現"], tests:["測試 →","← 被測試"],
    "in-bucket":["屬於 →","← 含"], "in-domain":["屬於 →","← 含"], depends:["相依 →","← 被相依"] };
  function colOf(d){ return d.kind==="file" ? (DOMAIN_COL[d.domain]||"#888")
    : (CONCEPT_COL[d.kind] || (d.kind==="domain"?(DOMAIN_COL[d.domain]||"#888"):"#7a8499")); }

  // ---- 度數（決定大小與標籤門檻）+ 鄰接索引 ----
  var deg = {}, OUT = {}, IN = {}, nodesById = {};
  DATA.elements.nodes.forEach(function(n){ nodesById[n.data.id]=n.data; deg[n.data.id]=0; });
  DATA.elements.edges.forEach(function(e){
    var d=e.data; deg[d.source]=(deg[d.source]||0)+1; deg[d.target]=(deg[d.target]||0)+1;
    (OUT[d.source]=OUT[d.source]||[]).push(d); (IN[d.target]=IN[d.target]||[]).push(d);
  });
  DATA.elements.nodes.forEach(function(n){
    var d=n.data, g=deg[d.id]||0;
    d.col = colOf(d);
    d.sz = d.kind==="file" ? 14+Math.min(30, g*1.1)
         : d.kind==="domain" ? 52 : d.kind==="bucket" ? 24 : 38;
    d.deg = g;
    d.shape = SHAPE[d.kind] || "ellipse";
  });

  // ---- Cytoscape ----
  var cy = cytoscape({
    container: document.getElementById("cy"),
    elements: DATA.elements,
    textureOnViewport: true, hideEdgesOnViewport: true, motionBlur: false,
    wheelSensitivity: 0.2, minZoom: 0.08, maxZoom: 4,
    style: [
      { selector:"node", style:{
        "background-color":"data(col)", "shape":"data(shape)",
        "width":"data(sz)", "height":"data(sz)",
        "label":"data(label)", "color":"#e6e6e6", "font-size":10, "font-weight":"bold",
        "font-family":'"Microsoft JhengHei","PingFang TC",sans-serif',
        "text-valign":"bottom", "text-halign":"center", "text-margin-y":4,
        "text-wrap":"wrap", "text-max-width":120, "min-zoomed-font-size":7,
        "border-width":2, "border-color":"#0f1419",
        "text-background-color":"#0f1419", "text-background-opacity":0.82, "text-background-padding":3,
        "text-opacity":0 } },
      { selector:'node[deg >= 8]', style:{ "text-opacity":1 } },
      { selector:'node[kind="pattern"],node[kind="principle"],node[kind="architecture"]',
        style:{ "text-opacity":1, "font-size":12 } },
      { selector:'node[kind="domain"]', style:{ "text-opacity":1, "font-size":13, "color":"#fff" } },
      // edges
      { selector:"edge", style:{ "curve-style":"haystack", "width":1.1, "line-color":"#344056", "opacity":0.32 } },
      { selector:'edge[etype="inherits"]', style:{ "curve-style":"bezier", "width":2.2,
        "line-color":"#4ade80", "target-arrow-shape":"triangle", "target-arrow-color":"#4ade80",
        "arrow-scale":0.9, "opacity":0.85 } },
      { selector:'edge[etype="realizes"]', style:{ "curve-style":"bezier", "width":1.8, "line-style":"dashed",
        "line-color":"#fbbf24", "target-arrow-shape":"vee", "target-arrow-color":"#fbbf24", "opacity":0.8 } },
      { selector:'edge[etype="depends"]', style:{ "curve-style":"bezier", "width":2.6, "line-style":"dashed",
        "line-color":"#f87171", "target-arrow-shape":"triangle", "target-arrow-color":"#f87171", "opacity":0.85 } },
      { selector:'edge[etype="tests"]', style:{ "line-color":"#5a4a63", "opacity":0.28 } },
      { selector:'edge[etype="in-bucket"],edge[etype="in-domain"]', style:{ "line-color":"#2a3046", "opacity":0.25 } },
      { selector:"node:selected", style:{ "border-width":4, "border-color":"#fbbf24" } },
      { selector:".faded", style:{ "opacity":0.1, "text-opacity":0 } },
      { selector:".search-hit", style:{ "border-width":3, "border-color":"#fbbf24", "text-opacity":1 } },
    ],
    layout: { name:"preset" },
  });
  window.cy = cy;
  var tooltip = document.getElementById("tooltip");

  // ---- 過濾狀態 ----
  var DOMAINS = DATA.meta.domains.slice();
  var kindShow = { file:true, concept:true, container:false };
  var domShow = {}; DOMAINS.forEach(function(d){ domShow[d]=true; });
  function kindBucket(k){ return k==="file" ? "file"
    : (k==="domain"||k==="bucket") ? "container" : "concept"; }
  function nodeVisible(d){
    if (!kindShow[kindBucket(d.kind)]) return false;
    if (d.kind==="file" && !domShow[d.domain]) return false;
    if (d.kind==="bucket" && !domShow[d.domain]) return false;
    return true;
  }
  function applyFilters(relayout){
    var vis = {};
    cy.batch(function(){
      cy.nodes().forEach(function(n){ var v=nodeVisible(n.data()); vis[n.id()]=v;
        n.style("display", v?"element":"none"); });
      cy.edges().forEach(function(e){ var d=e.data();
        e.style("display", (vis[d.source]&&vis[d.target])?"element":"none"); });
    });
    if (relayout) runLayout();
    updateStats();
  }

  // ---- 排版 ----
  function runLayout(){
    var name = (document.querySelector('input[name=layout]:checked')||{}).value || "cose";
    var eles = cy.elements(":visible");
    var opts = { name:name, fit:true, padding:30, animate:false };
    if (name==="cose") opts = Object.assign(opts, { idealEdgeLength:120, nodeRepulsion:13000,
      gravity:0.4, numIter:1000, componentSpacing:90 });
    else if (name==="concentric") opts = Object.assign(opts, { concentric:function(n){return n.degree();},
      levelWidth:function(){return 4;}, minNodeSpacing:14 });
    else if (name==="grid") opts = Object.assign(opts, { avoidOverlap:true,
      sort:function(a,b){ return (a.data("domain")||"z").localeCompare(b.data("domain")||"z"); } });
    eles.layout(opts).run();
  }

  // ---- hover：高亮鄰域 + tooltip ----
  cy.on("mouseover", "node", function(evt){
    var n = evt.target, hood = n.closedNeighborhood();
    cy.elements().difference(hood).addClass("faded");
    n.style("text-opacity", 1);
    var d = n.data(), oe = evt.originalEvent || {};
    tooltip.innerHTML = "<b>"+esc(d.label)+"</b>"
      + (d.path ? "<div class='tt-path'>"+esc(d.path)+"</div>" : "")
      + (d.summary ? "<div class='tt-path' style='color:#c5cdd6'>"+esc(d.summary.slice(0,80))+"…</div>" : "")
      + "<div class='tt-hint'>"+(d.deg||0)+" 條相依 · 點看詳情</div>";
    tooltip.style.display = "block";
    moveTip(oe);
  });
  cy.on("mousemove", "node", function(evt){ if (tooltip.style.display==="block") moveTip(evt.originalEvent||{}); });
  cy.on("mouseout", "node", function(evt){
    cy.elements().removeClass("faded");
    var d = evt.target.data(); if ((d.deg||0) < 8 && kindBucket(d.kind)==="file") evt.target.style("text-opacity", 0);
    tooltip.style.display = "none";
  });
  function moveTip(oe){
    var x = (oe.clientX||0), y = (oe.clientY||0);
    var tw = tooltip.offsetWidth, th = tooltip.offsetHeight;
    tooltip.style.left = Math.min(x+16, window.innerWidth - tw - 8) + "px";
    tooltip.style.top  = Math.min(y+16, window.innerHeight - th - 8) + "px";
  }

  // ---- tap：細節面板 ----
  cy.on("tap", "node", function(evt){
    showDetails(evt.target.data("id"));
    if (window.matchMedia("(max-width:768px)").matches) toggleSidebar(false);
  });
  cy.on("tap", function(evt){ if (evt.target===cy){ cy.elements().removeClass("faded"); } });
  cy.on("dbltap", "node", function(evt){
    var d = evt.target.data(); var u = d.wiki || d.github; if (u) window.open(u, "_blank");
  });
  cy.on("zoom", function(){
    var z = cy.zoom();
    cy.nodes().forEach(function(n){ var d=n.data();
      if (kindBucket(d.kind)!=="file") return;
      n.style("text-opacity", (z>1.3 || (d.deg||0)>=8) ? 1 : 0); });
  });

  function esc(s){ return String(s==null?"":s).replace(/[&<>"']/g,function(c){
    return {"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]; }); }

  function focusNode(id){ var n=cy.getElementById(id); if(!n.length) return;
    if (n.style("display")==="none"){ // 被過濾掉 → 打開它所屬種類/領域
      var d=n.data(); kindShow[kindBucket(d.kind)]=true; if(d.domain) domShow[d.domain]=true;
      syncControls(); applyFilters(false); }
    cy.elements().removeClass("faded"); cy.elements().difference(n.closedNeighborhood()).addClass("faded");
    cy.animate({ center:{eles:n}, zoom:Math.max(cy.zoom(),0.8) }, { duration:300 });
  }

  function relCard(e, otherId, outgoing){
    var o = nodesById[otherId]; if(!o) return "";
    var lbl = (ET[e.etype]||["→","←"])[outgoing?0:1];
    var links = [];
    if (o.github) links.push("<a href='"+o.github+"' target='_blank' rel='noopener'>↗ 原始碼</a>");
    if (o.wiki) links.push("<a href='"+o.wiki+"' target='_blank' rel='noopener'>📖 說明</a>");
    return "<div class='ref-card'>"
      + "<div class='head et-"+e.etype+"'>"+lbl+"</div>"
      + "<div class='title'><a data-focus='"+esc(o.id)+"'>"+esc(o.label)+"</a></div>"
      + (links.length?"<div class='links'>"+links.join("")+"</div>":"")
      + "</div>";
  }

  function showDetails(id){
    var d = nodesById[id]; if(!d) return;
    var det = document.getElementById("details"); det.style.display="block";
    document.getElementById("d-title").textContent = d.label;
    // tags
    var tags=[];
    if (d.kind==="file"){ tags.push(DOMAIN_LABEL[d.domain]||d.domain); if(d.bucket) tags.push(d.bucket);
      tags.push(d.ntype); if(d.loc) tags.push(d.loc+" 行"); }
    else tags.push(KIND_ZH[d.kind]||d.kind);
    document.getElementById("d-tags").innerHTML = tags.map(function(t){return "<span class='tag'>"+esc(t)+"</span>";}).join("");
    // summary (concepts)
    var sm = document.getElementById("d-summary");
    if (d.summary){ sm.style.display="block"; sm.textContent=d.summary; } else sm.style.display="none";
    // classes
    document.getElementById("d-classes").innerHTML = (d.classes&&d.classes.length)
      ? d.classes.map(function(c){return "<span class='chip'>"+esc(c)+"</span>";}).join("") : "";
    // buttons
    var btns=[];
    if (d.github) btns.push("<a class='src' href='"+d.github+"' target='_blank' rel='noopener'>↗ 原始碼</a>");
    if (d.wiki) btns.push("<a class='wiki' href='"+d.wiki+"' target='_blank' rel='noopener'>📖 逐檔詳盡說明</a>");
    document.getElementById("d-btns").innerHTML = btns.join("");
    // relationships
    var out = OUT[id]||[], inc = IN[id]||[];
    var oh = document.getElementById("d-out-h"), ih = document.getElementById("d-in-h");
    oh.textContent = d.kind==="file" ? "用到 / 體現的（往外）" : "落點檔案";
    ih.textContent = "被誰用到（往內）";
    document.getElementById("d-outgoing").innerHTML = out.length
      ? out.map(function(e){return relCard(e, e.target, true);}).join("")
      : "<div class='empty'>（無）</div>";
    document.getElementById("d-incoming").innerHTML = inc.length
      ? inc.map(function(e){return relCard(e, e.source, false);}).join("")
      : "<div class='empty'>（無）</div>";
    det.querySelectorAll("a[data-focus]").forEach(function(a){
      a.addEventListener("click", function(){ var fid=a.getAttribute("data-focus"); showDetails(fid); focusNode(fid); }); });
    det.scrollTop = 0;
  }

  // ---- 側欄控制 ----
  function updateStats(){
    var nf=0, nc=0; cy.nodes(":visible").forEach(function(n){ var b=kindBucket(n.data("kind")); if(b==="file")nf++; else if(b==="concept")nc++; });
    var ne = cy.edges(":visible").length;
    document.getElementById("stats").innerHTML =
      stat(nf,"顯示中的檔案") + stat(nc,"OO 概念") + stat(ne,"相依邊") + stat(DATA.meta.counts.files,"總檔案");
  }
  function stat(n,l){ return "<div class='stat'><div class='num'>"+n+"</div><div class='lbl'>"+l+"</div></div>"; }
  function syncControls(){
    // kinds
    var kc=[ ["file","程式檔"], ["concept","OO 概念（模式/原則/架構）"], ["container","領域 / bucket 容器"] ];
    document.getElementById("kinds").innerHTML = kc.map(function(k){
      return "<label><input type='checkbox' data-kind='"+k[0]+"' "+(kindShow[k[0]]?"checked":"")+"> "+k[1]+"</label>"; }).join("");
    document.querySelectorAll("#kinds input").forEach(function(cb){
      cb.addEventListener("change", function(){ kindShow[cb.dataset.kind]=cb.checked; applyFilters(true); }); });
    // domains
    var cnt={}; DATA.elements.nodes.forEach(function(n){ if(n.data.kind==="file") cnt[n.data.domain]=(cnt[n.data.domain]||0)+1; });
    document.getElementById("domains").innerHTML = DOMAINS.map(function(dm){
      return "<label><input type='checkbox' data-dom='"+dm+"' "+(domShow[dm]?"checked":"")+">"
        + "<span class='swatch' style='background:"+(DOMAIN_COL[dm]||"#888")+"'></span>"+(DOMAIN_LABEL[dm]||dm)
        + "<span class='cnt'>"+(cnt[dm]||0)+"</span></label>"; }).join("");
    document.querySelectorAll("#domains input").forEach(function(cb){
      cb.addEventListener("change", function(){ domShow[cb.dataset.dom]=cb.checked; applyFilters(false); }); });
  }

  document.querySelectorAll('input[name=layout]').forEach(function(r){
    r.addEventListener("change", runLayout); });
  document.getElementById("search").addEventListener("input", function(e){
    var q=e.target.value.trim().toLowerCase();
    cy.nodes().removeClass("search-hit");
    if(!q){ cy.elements().removeClass("faded"); return; }
    var hits=cy.nodes(":visible").filter(function(n){ var d=n.data();
      return (d.label&&d.label.toLowerCase().indexOf(q)>=0) || (d.path&&d.path.toLowerCase().indexOf(q)>=0)
        || (d.classes&&d.classes.join(" ").toLowerCase().indexOf(q)>=0); });
    cy.elements().addClass("faded"); hits.removeClass("faded").addClass("search-hit");
    hits.connectedEdges().removeClass("faded");
    if (hits.length && hits.length<14) cy.animate({ fit:{eles:hits, padding:60} }, {duration:300});
  });

  // ---- 行動版側欄 ----
  window.toggleSidebar = function(force){
    var a=document.getElementById("sidebar"), b=document.getElementById("sidebar-backdrop");
    var open=(typeof force==="boolean")?force:!a.classList.contains("open");
    a.classList.toggle("open",open); b.classList.toggle("open",open);
    setTimeout(function(){ cy.resize(); }, 280);
  };

  // ---- 啟動 ----
  if (!localStorage.getItem("xs_seen_intro")) document.getElementById("welcome").classList.add("show");
  document.getElementById("header-meta").textContent =
    DATA.meta.counts.files + " 個檔案 · " + DATA.meta.counts.edges_total + " 條相依";
  syncControls();
  applyFilters(false);
  runLayout();
  // 深連結 #node=<id>
  var m = /node=(.+)/.exec(location.hash);
  if (m){ var id=decodeURIComponent(m[1]); setTimeout(function(){ showDetails(id); focusNode(id); }, 500); }
})();
