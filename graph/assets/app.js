/* 《尋傘記》知識圖譜 — 互動邏輯（Cytoscape.js，手寫，無框架）。
   資料來自 window.GRAPH_DATA（data/graph-data.js）。 */
(function () {
  "use strict";

  var DATA = window.GRAPH_DATA;
  if (!DATA) { document.getElementById("cy").innerHTML =
    "<p style='padding:40px'>找不到 data/graph-data.js，請先跑 <code>python3 graph/build_graph.py</code>。</p>"; return; }

  // ---- 配色 ----
  var DOMAIN_COL = { app:"#e76f51", engine:"#2a9d8f", game:"#e9b949", ui:"#4895ef",
    tests:"#9aa5b1", docs:"#b5838d", tools:"#9c89b8", resources:"#74a12e", root:"#7b8794" };
  var KIND_COL = { pattern:"#f4a261", principle:"#06d6a0", architecture:"#118ab2" };
  var KIND_SHAPE = { file:"ellipse", bucket:"round-rectangle", domain:"round-rectangle",
    pattern:"star", principle:"diamond", architecture:"hexagon" };
  var KIND_LABEL = { file:"檔案", bucket:"bucket", domain:"領域分層",
    pattern:"設計模式 (GoF)", principle:"OO 原則 / 技法", architecture:"架構元件" };
  var EDGE_COL = { includes:"#cbd5e1", inherits:"#3a86ff", realizes:"#f4a261",
    "in-bucket":"#e4ebf3", "in-domain":"#e4ebf3", depends:"#ef476f", tests:"#b8c0cc" };
  var EDGE_LABEL = { includes:"#include 相依", inherits:"類別繼承", realizes:"模式落點",
    "in-bucket":"歸屬 bucket", "in-domain":"歸屬領域", depends:"領域依賴方向", tests:"測試對應" };

  // ---- 預先標註每個節點的顏色 / 大小 ----
  DATA.elements.nodes.forEach(function (n) {
    var d = n.data;
    if (d.kind === "file") { d.col = DOMAIN_COL[d.domain] || "#888"; d.sz = 14 + Math.min(28, (d.loc || 0) / 22); }
    else if (d.kind === "domain") { d.col = DOMAIN_COL[d.domain] || "#555"; d.sz = 54; }
    else if (d.kind === "bucket") { d.col = DOMAIN_COL[d.domain] || "#888"; d.sz = 26; }
    else { d.col = KIND_COL[d.kind] || "#888"; d.sz = 36; }
  });

  // ---- 建立 Cytoscape ----
  var cy = cytoscape({
    container: document.getElementById("cy"),
    elements: DATA.elements,
    wheelSensitivity: 0.25,
    textureOnViewport: true,    // 拖曳/縮放用貼圖渲染 → 大圖不卡（關鍵）
    hideEdgesOnViewport: true,  // 互動時暫時隱藏邊，少畫很多東西
    motionBlur: false,
    style: [
      { selector: "node", style: {
        "background-color": "data(col)", "shape": function (e){ return KIND_SHAPE[e.data("kind")] || "ellipse"; },
        "width": "data(sz)", "height": "data(sz)", "label": "data(label)",
        "font-size": 8, "color": "#1f2933", "text-wrap": "wrap", "text-max-width": 90,
        "text-valign": "bottom", "text-margin-y": 2, "min-zoomed-font-size": 7,
        "border-width": 0, "text-opacity": 0.0 } },
      { selector: 'node[kind="file"]', style: { "text-opacity": 0.0 } },
      { selector: 'node[kind="bucket"]', style: { "text-opacity": 0.85, "font-size": 10 } },
      { selector: 'node[kind="domain"]', style: { "text-opacity": 1, "font-size": 14,
        "font-weight": "bold", "color": "#fff", "text-valign": "center", "text-outline-width": 0 } },
      { selector: 'node[kind="pattern"],node[kind="principle"],node[kind="architecture"]',
        style: { "text-opacity": 1, "font-size": 11, "font-weight": "bold" } },
      { selector: "edge", style: {
        "width": 1, "line-color": "data(col)", "curve-style": "haystack",
        "haystack-radius": 0, "opacity": 0.55 } },
      { selector: 'edge[etype="inherits"]', style: { "width": 2.4, "curve-style": "bezier",
        "target-arrow-shape": "triangle", "target-arrow-color": EDGE_COL.inherits, "opacity": 0.9 } },
      { selector: 'edge[etype="realizes"]', style: { "width": 2, "curve-style": "bezier",
        "line-style": "dashed", "target-arrow-shape": "vee", "target-arrow-color": EDGE_COL.realizes, "opacity": 0.9 } },
      { selector: 'edge[etype="depends"]', style: { "width": 3, "curve-style": "bezier",
        "line-style": "dashed", "target-arrow-shape": "triangle", "target-arrow-color": EDGE_COL.depends, "opacity": 0.85 } },
      { selector: 'edge[etype="tests"]', style: { "line-style": "dotted", "opacity": 0.4 } },
      { selector: ".faded", style: { "opacity": 0.07, "text-opacity": 0.0 } },
      { selector: ".hl", style: { "text-opacity": 1, "font-size": 11, "z-index": 99,
        "border-width": 2, "border-color": "#1f2933" } },
      { selector: ".search-hit", style: { "background-color": "#ef233c", "text-opacity": 1,
        "border-width": 3, "border-color": "#ef233c", "z-index": 100 } },
      { selector: "edge.hl", style: { "opacity": 1, "width": 3 } },
    ],
    layout: { name: "preset" },
  });

  // ---- 鄰接索引（給 detail 面板用）----
  var IN = {}, OUT = {};   // OUT[src] = [{etype,target}], IN[tgt] = [{etype,source}]
  DATA.elements.edges.forEach(function (e) {
    var d = e.data; (OUT[d.source] = OUT[d.source] || []).push(d);
    (IN[d.target] = IN[d.target] || []).push(d);
  });
  function nodeData(id){ var n = cy.getElementById(id); return n.length ? n.data() : null; }

  // ---- 篩選狀態 ----
  var kinds = ["domain","bucket","pattern","principle","architecture","file"];
  var domains = DATA.meta.domains.slice();
  var etypes = DATA.meta.edge_types.slice();
  var kindOn = {}, domOn = {}, edgeOn = {};
  kinds.forEach(function(k){ kindOn[k] = true; });
  domains.forEach(function(d){ domOn[d] = true; });
  etypes.forEach(function(t){ edgeOn[t] = true; });

  function nodeVisible(d){
    if (!kindOn[d.kind]) return false;
    if (d.kind === "file" && !domOn[d.domain]) return false;
    if (d.kind === "bucket" && !domOn[d.domain]) return false;
    return true;
  }
  function applyFilters(relayout){
    var vis = {};   // 先算進 JS map，別在 batch 內回讀 cy style（可能 stale）
    cy.batch(function(){
      cy.nodes().forEach(function(n){
        var v = nodeVisible(n.data()); vis[n.id()] = v;
        n.style("display", v ? "element" : "none");
      });
      cy.edges().forEach(function(e){
        var d = e.data(), ok = edgeOn[d.etype] && vis[d.source] && vis[d.target];
        e.style("display", ok ? "element" : "none");
      });
    });
    if (relayout) runLayout();
    updateStats();
  }

  // ---- 排版 ----
  function layoutOpts(name){
    var vis = cy.elements(":visible");
    var common = { name: name, fit: true, padding: 40, animate: false };
    if (name === "cose") return Object.assign(common, { nodeRepulsion: 9000,
      idealEdgeLength: 70, numIter: 1200, gravity: 0.3, componentSpacing: 80 });
    if (name === "concentric") return Object.assign(common, {
      concentric: function(n){ return n.degree(); }, levelWidth: function(){ return 3; }, minNodeSpacing: 18 });
    if (name === "breadthfirst") return Object.assign(common, { directed: true, spacingFactor: 1.1 });
    if (name === "grid") return Object.assign(common, { avoidOverlap: true });
    if (name === "circle") return common;
    return common;
  }
  function runLayout(){
    var name = document.getElementById("layout").value;
    cy.elements(":visible").layout(layoutOpts(name)).run();
  }

  // ---- 預設檢視 ----
  var VIEWS = {
    skeleton: { kinds:["domain","bucket","pattern","principle","architecture"],
      edges:["in-domain","in-bucket","depends","realizes"], layout:"cose" },
    inherit:  { kinds:["file","pattern","principle","architecture"],
      edges:["inherits","realizes"], layout:"breadthfirst" },
    patterns: { kinds:["pattern","principle","architecture","file"],
      edges:["realizes"], layout:"concentric" },
    code:     { kinds:["file"], domains:["app","engine","game","ui"],
      edges:["includes","inherits"], layout:"cose" },
    full:     { kinds:kinds.slice(), edges:etypes.slice(), layout:"cose" },
  };
  function setView(name){
    var v = VIEWS[name]; if (!v) return;
    kinds.forEach(function(k){ kindOn[k] = v.kinds.indexOf(k) >= 0; });
    etypes.forEach(function(t){ edgeOn[t] = v.edges.indexOf(t) >= 0; });
    domains.forEach(function(d){ domOn[d] = v.domains ? v.domains.indexOf(d) >= 0 : true; });
    document.getElementById("layout").value = v.layout;
    syncCheckboxes();
    document.querySelectorAll("#views button").forEach(function(b){
      b.classList.toggle("active", b.dataset.view === name); });
    applyFilters(true);
  }

  // ---- 細節面板 ----
  var app = document.getElementById("app"), detail = document.getElementById("detail");
  function fileName(id){ var d = nodeData(id); return d ? d.label : id; }
  function linkList(arr, dir){
    if (!arr || !arr.length) return "<li style='color:#9aa5b1'>（無）</li>";
    return arr.map(function(e){
      var other = dir === "out" ? e.target : e.source;
      return "<li><span class='chip' style='background:#f1f5f9'>"+ (EDGE_LABEL[e.etype]||e.etype)
        +"</span> <a class='ln' data-go='"+other+"'>"+ fileName(other) +"</a></li>";
    }).join("");
  }
  function showDetail(id){
    var d = nodeData(id); if (!d) return;
    var kindCol = d.kind === "file" ? (DOMAIN_COL[d.domain]||"#777") : (KIND_COL[d.kind]||"#777");
    document.getElementById("dKind").textContent = KIND_LABEL[d.kind] || d.kind;
    document.getElementById("dKind").style.background = kindCol;
    document.getElementById("dTitle").textContent = d.label;
    document.getElementById("dPath").textContent = d.path || d.id;
    var h = [];
    var meta = [];
    if (d.kind === "file"){
      meta.push(["領域", d.domain + (d.bucket ? " / " + d.bucket : "")]);
      meta.push(["類型", d.ntype]); if (d.loc) meta.push(["行數", d.loc + " LOC"]);
    } else if (d.kind === "domain" || d.kind === "bucket"){
      meta.push(["種類", KIND_LABEL[d.kind]]); meta.push(["領域", d.domain]);
    }
    if (meta.length) h.push("<dl class='meta'>" + meta.map(function(m){
      return "<dt>"+m[0]+"</dt><dd>"+m[1]+"</dd>"; }).join("") + "</dl>");
    if (d.summary) h.push("<div class='summary'>"+ d.summary +"</div>");
    if (d.classes && d.classes.length) h.push("<h4>類別</h4><div>" +
      d.classes.map(function(c){ return "<span class='chip'>"+c+"</span>"; }).join("") + "</div>");

    var out = OUT[id] || [], inc = IN[id] || [];
    var inclOut = out.filter(function(e){return e.etype==="includes";});
    var inclIn = inc.filter(function(e){return e.etype==="includes";});
    var inhOut = out.filter(function(e){return e.etype==="inherits";});
    var inhIn = inc.filter(function(e){return e.etype==="inherits";});
    var realIn = inc.filter(function(e){return e.etype==="realizes";});
    var realOut = out.filter(function(e){return e.etype==="realizes";});
    if (inhOut.length) h.push("<h4>繼承自</h4><ul class='links'>"+linkList(inhOut,"out")+"</ul>");
    if (inhIn.length) h.push("<h4>被繼承（子類別）</h4><ul class='links'>"+linkList(inhIn,"in")+"</ul>");
    if (realOut.length) h.push("<h4>落點檔案</h4><ul class='links'>"+linkList(realOut,"out")+"</ul>");
    if (realIn.length) h.push("<h4>體現的概念 / 模式</h4><ul class='links'>"+linkList(realIn,"in")+"</ul>");
    if (inclOut.length) h.push("<h4>#include（"+inclOut.length+"）</h4><ul class='links'>"+linkList(inclOut,"out")+"</ul>");
    if (inclIn.length) h.push("<h4>被誰 include（"+inclIn.length+"）</h4><ul class='links'>"+linkList(inclIn,"in")+"</ul>");

    var ext = [];
    if (d.github) ext.push("<a href='"+d.github+"' target='_blank' rel='noopener'>↗ 原始碼 (GitHub)</a>");
    if (d.wiki) ext.push("<a class='ghost' href='"+d.wiki+"' target='_blank'>📖 wiki</a>");
    if (ext.length) h.push("<div class='ext'>"+ext.join("")+"</div>");
    document.getElementById("dBody").innerHTML = h.join("");

    detail.classList.remove("hidden"); app.classList.add("detail-open");
    document.querySelectorAll("#dBody a.ln").forEach(function(a){
      a.addEventListener("click", function(){ selectNode(a.dataset.go); }); });
  }
  function closeDetail(){ detail.classList.add("hidden"); app.classList.remove("detail-open");
    cy.elements().removeClass("faded hl"); location.hash = ""; }
  document.getElementById("closeDetail").addEventListener("click", closeDetail);

  // ---- 選取 / 聚焦 ----
  function focusNode(node){
    cy.elements().addClass("faded").removeClass("hl");
    var hood = node.closedNeighborhood();
    hood.removeClass("faded"); hood.addClass("hl"); node.addClass("hl");
    cy.animate({ center: { eles: node }, zoom: Math.max(cy.zoom(), 0.9) }, { duration: 300 });
  }
  function selectNode(id){
    var node = cy.getElementById(id);
    if (!node.length) return;
    if (node.style("display") === "none") setView("full");   // 被預設檢視藏起來 → 切全圖
    node = cy.getElementById(id);
    focusNode(node); showDetail(id);
    var enc = encodeURIComponent(id);
    if (location.hash !== "#node=" + enc) location.hash = "node=" + enc;
  }

  var lastTap = { id: null, t: 0 };   // 手動偵測雙擊（cytoscape 核心無保證的 dbltap）
  cy.on("tap", "node", function(evt){
    var id = evt.target.id(), now = Date.now();
    if (lastTap.id === id && now - lastTap.t < 320){          // 雙擊 → 開原始碼
      var g = evt.target.data("github"); if (g) window.open(g, "_blank");
      lastTap.t = 0; return;
    }
    lastTap = { id: id, t: now };
    selectNode(id);
  });
  cy.on("tap", function(evt){ if (evt.target === cy){ cy.elements().removeClass("faded hl"); } });

  // ---- 搜尋 ----
  document.getElementById("search").addEventListener("input", function(e){
    var q = e.target.value.trim().toLowerCase();
    cy.nodes().removeClass("search-hit");
    if (!q){ return; }
    var hits = cy.nodes().filter(function(n){
      var d = n.data();
      return (d.label && d.label.toLowerCase().indexOf(q) >= 0)
        || (d.path && d.path.toLowerCase().indexOf(q) >= 0)
        || (d.classes && d.classes.join(" ").toLowerCase().indexOf(q) >= 0);
    });
    // 確保命中節點可見
    var needFull = hits.some(function(n){ return n.style("display") === "none"; });
    if (needFull && hits.length) setView("full");
    hits = hits.filter(function(n){ return n.style("display") !== "none"; });
    hits.addClass("search-hit");
    if (hits.length) cy.animate({ fit: { eles: hits, padding: 80 } }, { duration: 300 });
  });

  // ---- 側欄：核取方塊 ----
  function makeChecks(containerId, items, state, colorOf, onToggle){
    var c = document.getElementById(containerId);
    c.innerHTML = "";
    items.forEach(function(it){
      var id = it.key, lbl = it.label, col = colorOf ? colorOf(id) : null;
      var w = document.createElement("label"); w.className = "chk";
      w.innerHTML = (col ? "<span class='dot' style='background:"+col+"'></span>" : "")
        + "<input type='checkbox' "+(state[id]?"checked":"")+"> "+lbl
        + "<span class='count'>"+(it.count!=null?it.count:"")+"</span>";
      w.querySelector("input").addEventListener("change", function(ev){
        state[id] = ev.target.checked; onToggle(); });
      c.appendChild(w);
    });
  }
  function counts(pred){ return cy.nodes().filter(pred).length; }
  function syncCheckboxes(){
    makeChecks("kindFilters", kinds.map(function(k){ return { key:k, label:KIND_LABEL[k],
      count: counts(function(n){return n.data("kind")===k;}) }; }), kindOn,
      function(k){ return KIND_COL[k] || DOMAIN_COL[k] || "#999"; }, function(){ applyFilters(false); });
    makeChecks("domainFilters", domains.map(function(d){ return { key:d, label:d,
      count: counts(function(n){return n.data("kind")==="file"&&n.data("domain")===d;}) }; }),
      domOn, function(d){ return DOMAIN_COL[d]; }, function(){ applyFilters(false); });
    makeChecks("edgeFilters", etypes.map(function(t){ return { key:t,
      label:EDGE_LABEL[t]||t, count: (cy.edges('[etype="'+t+'"]').length) }; }),
      edgeOn, function(t){ return EDGE_COL[t]; }, function(){ applyFilters(false); });
  }

  function buildLegend(){
    var L = document.getElementById("legend"); var h = [];
    h.push("<div style='margin-bottom:6px'><strong>節點顏色＝領域</strong></div>");
    ["app","engine","game","ui","tests","docs","tools","resources","root"].forEach(function(d){
      h.push("<div class='row'><span class='swatch' style='background:"+DOMAIN_COL[d]+"'></span>"+d+"</div>"); });
    h.push("<div style='margin:8px 0 6px'><strong>概念節點（形狀）</strong></div>");
    h.push("<div class='row'><span class='swatch' style='background:"+KIND_COL.pattern+"'></span>★ 設計模式</div>");
    h.push("<div class='row'><span class='swatch' style='background:"+KIND_COL.principle+"'></span>◆ OO 原則</div>");
    h.push("<div class='row'><span class='swatch' style='background:"+KIND_COL.architecture+"'></span>⬡ 架構元件</div>");
    h.push("<div style='margin:8px 0 6px'><strong>邊</strong></div>");
    ["inherits","realizes","includes","depends","tests"].forEach(function(t){
      h.push("<div class='row'><span class='edge' style='border-top:3px "+
        (t==="realizes"||t==="depends"?"dashed":t==="tests"?"dotted":"solid")+" "+EDGE_COL[t]+"'></span>"+(EDGE_LABEL[t]||t)+"</div>"); });
    L.innerHTML = h.join("");
  }
  function updateStats(){
    var vn = cy.nodes(":visible").length, ve = cy.edges(":visible").length;
    document.getElementById("stats").innerHTML =
      "顯示中："+vn+" 節點 · "+ve+" 邊<br>全圖："+DATA.meta.counts.nodes_total+" 節點 · "
      + DATA.meta.counts.edges_total + " 邊（涵蓋 "+DATA.meta.counts.files+" 個檔案）"
      + "<br>產生：<code>graph/build_graph.py</code>";
  }

  // ---- 控制項綁定 ----
  document.querySelectorAll("#views button").forEach(function(b){
    b.addEventListener("click", function(){ setView(b.dataset.view); }); });
  document.getElementById("layout").addEventListener("change", runLayout);
  document.getElementById("refit").addEventListener("click", function(){ cy.fit(null, 40); });
  window.addEventListener("hashchange", function(){
    var m = /node=(.+)/.exec(location.hash); if (m) selectNode(decodeURIComponent(m[1])); });

  // ---- 啟動 ----
  buildLegend();
  syncCheckboxes();
  setView("skeleton");                       // 預設：乾淨的架構骨架
  var m = /node=(.+)/.exec(location.hash);   // 深連結：#node=ID
  if (m) setTimeout(function(){ selectNode(decodeURIComponent(m[1])); }, 400);
})();
