const $ = (id) => document.getElementById(id);
const svgNamespace = "http://www.w3.org/2000/svg";
const titles = {
    ready: "Готово",
    start: "Начинаем операцию",
    compare: "Выбираем ветвь",
    insert: "Новый узел",
    duplicate: "Такой узел уже есть",
    found: "Узел найден",
    missing: "Узел не найден",
    removing: "Удаляем узел",
    detach: "Находим замену",
    remove: "Проверяем предков",
    imbalance: "Восстанавливаем баланс",
    "rotate-left": "Поворот влево",
    "rotate-right": "Поворот вправо",
    visit: "Посещаем узел",
    done: "Готово",
};
const icons = {
    ready: "↳",
    start: "↳",
    compare: "↳",
    insert: "+",
    duplicate: "=",
    found: "✓",
    missing: "∅",
    removing: "−",
    detach: "↗",
    remove: "−",
    imbalance: "!",
    "rotate-left": "↑",
    "rotate-right": "↑",
    visit: "✓",
    done: "✓",
};
let frames = [];
let layouts = [];
let frameIndex = 0;
let tabVersion = 0;
let operation = "add";
let loading = false;
let playing = false;
let playTimer;
let errorTimer;
let animation;
let cleanupTimer;
let camera = { x: -400, y: -70, width: 800, height: 420 };
const nodeElements = new Map();
const edgeElements = new Map();
const positions = new Map();

function CreateSvg(name, attributes = {}) {
    const element = document.createElementNS(svgNamespace, name);
    for (const [key, value] of Object.entries(attributes)) element.setAttribute(key, value);
    return element;
}

function Layout(nodes) {
    const byValue = new Map(nodes.map((node) => [node.value, node]));
    const result = new Map();
    let rank = 0;
    function Visit(value, depth) {
        if (value === null || !byValue.has(value)) return;
        const node = byValue.get(value);
        Visit(node.left, depth + 1);
        result.set(value, { x: (rank++ - (nodes.length - 1) / 2) * 88, y: depth * 108 });
        Visit(node.right, depth + 1);
    }
    if (nodes.length) Visit(nodes[0].value, 0);
    return result;
}

function LoadFrames(nextFrames) {
    frames = nextFrames;
    layouts = [];
    for (let i = 0; i < frames.length; i++) {
        const previous = frames[i - 1]?.nodes;
        const nodes = frames[i].nodes;
        const sameStructure =
            previous?.length === nodes.length &&
            nodes.every((node, index) => {
                const other = previous[index];
                return node.value === other.value && node.left === other.left && node.right === other.right;
            });
        layouts.push(sameStructure ? layouts[i - 1] : Layout(nodes));
    }
    FitTree();
}

function ReadRotation(index) {
    const finished = frames[index].action === "done";
    let turn = finished ? index - 1 : index;
    if (frames[index].action === "imbalance") turn++;
    if (!frames[turn]?.action.startsWith("rotate-")) return null;

    // Both parts of LR/RL belong to the same three original nodes.
    let first = turn;
    let last = turn;
    while (frames[first - 1]?.action.startsWith("rotate-")) first--;
    while (frames[last + 1]?.action.startsWith("rotate-")) last++;
    const firstTurn = frames[first];
    const double = last > first;
    const kind = double
        ? firstTurn.action === "rotate-left" ? "LR" : "RL"
        : firstTurn.action === "rotate-left" ? "RR" : "LL";
    const initial = new Map(frames[first - 1].nodes.map((node) => [node.value, node]));
    const keys = double
        ? [frames[last].related, firstTurn.related, firstTurn.current]
        : [firstTurn.related, firstTurn.current];

    const frame = frames[turn];
    const before = new Map(frames[turn - 1].nodes.map((node) => [node.value, node]));
    const promoted = before.get(frame.current);
    const left = frame.action === "rotate-left";
    return {
        kind,
        keys,
        count: last - first + 1,
        phase: index < first ? 0 : turn - first + 1,
        finished,
        root: frames[last].current,
        balance: initial.get(keys[0]).balance,
        pivot: frame.related,
        promoted: frame.current,
        left,
        transfer: left ? promoted.left : promoted.right,
    };
}

function DescribeRotation(rotation) {
    if (rotation.finished) {
        return [`${rotation.kind} · Готово`, `${rotation.root} — корень поддерева. Баланс восстановлен.`];
    }
    if (rotation.phase === 0) {
        const balance = rotation.balance > 0 ? `+${rotation.balance}` : rotation.balance;
        return [
            `${rotation.kind} · Нарушен баланс`,
            `Узел ${rotation.keys[0]}: баланс ${balance}. ${rotation.root} займёт его место.`,
        ];
    }
    const phase = rotation.count === 2 ? ` · ${rotation.phase}/2` : "";
    const side = rotation.left ? "левым" : "правым";
    const transfer = rotation.transfer === null
        ? ""
        : ` Ветка ${rotation.transfer}: родитель ${rotation.promoted} → ${rotation.pivot}.`;
    return [
        `${rotation.kind}${phase} — поднимаем ${rotation.promoted}`,
        `${rotation.pivot} становится ${side} ребёнком ${rotation.promoted}.${transfer}`,
    ];
}

function UpdateCamera() {
    $("tree").setAttribute("viewBox", `${camera.x} ${camera.y} ${camera.width} ${camera.height}`);
}

function FitTree() {
    let minX = Infinity;
    let maxX = -Infinity;
    let maxY = 120;
    for (const layout of new Set(layouts)) {
        for (const point of layout.values()) {
            minX = Math.min(minX, point.x - 95);
            maxX = Math.max(maxX, point.x + 95);
            maxY = Math.max(maxY, point.y + 100);
        }
    }
    if (!Number.isFinite(minX)) {
        minX = -200;
        maxX = 200;
        maxY = 220;
    }
    const minY = -70;
    const aspect = $("tree").clientWidth / Math.max(1, $("tree").clientHeight);
    const width = Math.max(maxX - minX, (maxY - minY) * aspect);
    const height = width / aspect;
    camera = { x: (minX + maxX - width) / 2, y: (minY + maxY - height) / 2, width, height };
    UpdateCamera();
}

function Zoom(factor, horizontal = 0.5, vertical = 0.5) {
    const width = Math.max(180, Math.min(1000000, camera.width * factor));
    const height = (width * camera.height) / camera.width;
    camera.x += (camera.width - width) * horizontal;
    camera.y += (camera.height - height) * vertical;
    camera.width = width;
    camera.height = height;
    UpdateCamera();
}

function MakeNode(value) {
    const group = CreateSvg("g", { class: "node", "data-value": value });
    group.append(
        CreateSvg("title"),
        CreateSvg("circle", { class: "halo", r: 36 }),
        CreateSvg("circle", { class: "body", r: 26 }),
        CreateSvg("text", { class: "value", y: 0 }),
        CreateSvg("rect", { class: "meta-backdrop", x: -32, y: 34, width: 64, height: 18, rx: 6 }),
        CreateSvg("text", { class: "meta", y: 48 }),
    );
    group.querySelector(".value").textContent = value;
    group.addEventListener("click", () => {
        $("value").value = value;
    });
    $("nodes").append(group);
    nodeElements.set(value, group);
    return group;
}

function InterpolatePoint(start, end, progress) {
    return {
        x: start.x + (end.x - start.x) * progress,
        y: start.y + (end.y - start.y) * progress,
    };
}

function PrepareEdges(nodes, target) {
    const remaining = new Map(edgeElements);
    const connections = [];
    for (const node of nodes) {
        for (const child of [node.left, node.right]) {
            if (child === null) continue;
            const key = `${Math.min(node.value, child)}:${Math.max(node.value, child)}`;
            connections.push({ key, parent: node.value, child, edge: remaining.get(key) });
            remaining.delete(key);
        }
    }
    // Keep unchanged segments first, then move the loose end of a rewired segment.
    for (const connection of connections) {
        if (connection.edge) continue;
        const candidates = [...remaining];
        const match = candidates.find(([, edge]) => edge.child === connection.child)
            || candidates.find(([, edge]) => edge.parent === connection.parent)
            || candidates.find(([, edge]) => edge.parent === connection.child || edge.child === connection.parent)
            || candidates[0];
        if (!match) continue;
        connection.edge = match[1];
        remaining.delete(match[0]);
    }
    const motions = [];
    edgeElements.clear();
    for (const { key, parent, child, edge: previous } of connections) {
        const edge = previous || { element: CreateSvg("path", { class: "edge" }) };
        if (!previous) $("edges").append(edge.element);
        let from = edge.from;
        let to = edge.to;
        let sourceParent = previous ? edge.parent : parent;
        let sourceChild = previous ? edge.child : child;
        if (previous && (edge.parent === child || edge.child === parent)) {
            [from, to] = [to, from];
            [sourceParent, sourceChild] = [sourceChild, sourceParent];
        }
        from = from || positions.get(parent) || target.get(parent);
        to = to || from;
        motions.push({
            edge,
            startFrom: { ...from },
            startTo: { ...to },
            sourceParent,
            sourceChild,
            targetParent: parent,
            targetChild: child,
        });
        edge.parent = parent;
        edge.child = child;
        edgeElements.set(key, edge);
    }
    for (const [key, edge] of remaining) {
        const anchor = target.has(edge.parent) ? edge.parent
            : target.has(edge.child) ? edge.child : edge.parent;
        motions.push({
            edge,
            startFrom: { ...edge.from },
            startTo: { ...edge.to },
            sourceParent: edge.parent,
            sourceChild: edge.child,
            targetParent: anchor,
            targetChild: anchor,
        });
        edgeElements.set(key, edge);
    }
    return { motions, activeEdges: new Set(connections.map((connection) => connection.key)) };
}

function DrawTree(frame, duration, rotation) {
    cancelAnimationFrame(animation);
    const target = layouts[frameIndex];
    const visited = new Set(frame.visited);
    for (let i = 0; i < frameIndex; i++) {
        if (frames[i].action === "compare") visited.add(frames[i].current);
    }
    const activeNodes = new Set(frame.nodes.map((node) => node.value));
    const parents = new Map();
    for (const node of frame.nodes) {
        for (const child of [node.left, node.right]) {
            if (child !== null) parents.set(child, node.value);
        }
    }
    for (const node of frame.nodes) {
        const group = nodeElements.get(node.value) || MakeNode(node.value);
        if (!positions.has(node.value)) {
            const parent = parents.get(node.value);
            positions.set(node.value, { ...(positions.get(parent) || target.get(node.value)), opacity: 0 });
        }
        const classes = ["node"];
        if (rotation) {
            if (rotation.keys.includes(node.value)) classes.push("rotation-node");
            const active = rotation.finished
                ? node.value === rotation.root
                : rotation.phase > 0 && [rotation.pivot, rotation.promoted].includes(node.value);
            if (active) classes.push("rotation-active");
        } else {
            if (Math.abs(node.balance) > 1) classes.push("unbalanced");
            if (visited.has(node.value)) classes.push("visited");
            if (node.value === frame.current) {
                classes.push("current");
                if (frame.action === "found") classes.push("found");
                if (frame.action === "imbalance" || frame.action === "removing") classes.push("imbalance");
            }
        }
        group.setAttribute("class", classes.join(" "));
        group.dataset.balance = node.balance;
        group.querySelector(".meta").textContent =
            `h ${node.height} · b ${node.balance > 0 ? "+" : ""}${node.balance}`;
        group.querySelector("title").textContent =
            `Узел ${node.value}. Высота ${node.height}, баланс ${node.balance}. Нажми, чтобы выбрать число.`;
    }
    const starts = new Map([...positions].map(([value, point]) => [value, { ...point }]));
    const { motions, activeEdges } = PrepareEdges(frame.nodes, target);
    for (const edge of edgeElements.values()) {
        const classes = ["edge"];
        if (rotation) {
            if (rotation.keys.includes(edge.parent) && rotation.keys.includes(edge.child)) {
                classes.push("rotation-link");
            }
            if (rotation.phase > 0 && !rotation.finished && edge.child === rotation.transfer &&
                [rotation.pivot, rotation.promoted].includes(edge.parent)) {
                classes.push("transfer-link");
            }
        } else {
            if (visited.has(edge.parent) && visited.has(edge.child)) classes.push("visited");
            if (edge.child === frame.current) classes.push("active");
        }
        edge.element.setAttribute("class", classes.join(" "));
    }
    const startTime = performance.now();
    function MoveAttachment(start, source, destination, progress) {
        const initial = starts.get(source);
        const current = positions.get(source);
        // Preserve an endpoint's current position when a step interrupts its motion.
        const carried = {
            x: start.x + current.x - initial.x,
            y: start.y + current.y - initial.y,
        };
        return InterpolatePoint(carried, positions.get(destination), progress);
    }
    function Paint(now) {
        const progress = duration ? Math.min(1, (now - startTime) / duration) : 1;
        const eased = rotation ? progress * progress * (3 - 2 * progress) : 1 - Math.pow(1 - progress, 3);
        for (const [value, start] of starts) {
            const end = target.get(value) || { x: start.x, y: start.y + 20 };
            const point = InterpolatePoint(start, end, eased);
            point.opacity = start.opacity + ((activeNodes.has(value) ? 1 : 0) - start.opacity) * eased;
            positions.set(value, point);
            const group = nodeElements.get(value);
            group.setAttribute("transform", `translate(${point.x} ${point.y})`);
            group.setAttribute("opacity", point.opacity);
        }
        for (const motion of motions) {
            const edge = motion.edge;
            edge.from = MoveAttachment(motion.startFrom, motion.sourceParent, motion.targetParent, eased);
            edge.to = MoveAttachment(motion.startTo, motion.sourceChild, motion.targetChild, eased);
            edge.element.setAttribute(
                "d",
                `M${edge.from.x},${edge.from.y} L${edge.to.x},${edge.to.y}`,
            );
        }
        if (progress < 1) animation = requestAnimationFrame(Paint);
        else {
            for (const [value, group] of nodeElements) {
                if (activeNodes.has(value)) continue;
                group.remove();
                nodeElements.delete(value);
                positions.delete(value);
            }
            for (const [key, edge] of edgeElements) {
                if (activeEdges.has(key)) continue;
                edge.element.remove();
                edgeElements.delete(key);
            }
        }
    }
    Paint(startTime);
    $("empty-state").hidden = frame.nodes.length > 0;
}

function UpdateControls() {
    const last = frames.length - 1;
    $("previous").disabled = loading || frameIndex <= 0;
    $("next").disabled = loading || frameIndex >= last;
    $("play").disabled = loading || frames.length <= 1;
    $("progress").disabled = loading || frames.length <= 1;
    $("play").setAttribute(
        "aria-label",
        playing ? "Пауза" : frames.length > 1 && frameIndex === last ? "Повторить" : "Воспроизвести",
    );
    $("play-icon").setAttribute("href", playing ? "#i-pause" : "#i-play");
    document
        .querySelectorAll(
            ".control-section button, .control-section input, .control-section select, .sidebar-bottom button",
        )
        .forEach((element) => {
            element.disabled = loading || playing || frameIndex < last;
        });
    $("operation-lock").hidden = frameIndex >= last;
}

function ShowFrame(index, animate = true) {
    if (!frames.length) return;
    clearTimeout(cleanupTimer);
    $("tree").classList.remove("settling");
    frameIndex = Math.max(0, Math.min(frames.length - 1, index));
    const frame = frames[frameIndex];
    const speed = Number($("speed").value);
    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const rotation = ReadRotation(frameIndex);
    const duration = animate && !reducedMotion ? (rotation?.phase > 0 ? 1500 : 580) / speed : 0;
    const moving = [...layouts[frameIndex]].some(([value, end]) => {
        const start = positions.get(value);
        return !start || Math.abs(start.x - end.x) > 0.5 || Math.abs(start.y - end.y) > 0.5;
    });
    DrawTree(frame, duration, rotation);
    $("node-count").textContent = frame.nodes.length;
    $("tree-height").textContent = frame.nodes[0]?.height ?? 0;
    $("rotation-count").textContent = frames
        .slice(0, frameIndex + 1)
        .filter((step) => step.action.startsWith("rotate-")).length;
    $("action-title").textContent = titles[frame.action];
    $("action-message").textContent = ["ready", "done"].includes(frame.action) ? "" : frame.message;
    if (rotation) {
        const [title, message] = DescribeRotation(rotation);
        $("action-title").textContent = title;
        $("action-message").textContent = message;
    }
    $("action-message").hidden = !$("action-message").textContent;
    $("action-icon").textContent = icons[frame.action] || "↳";
    $("action-icon").classList.toggle("rotation", rotation || frame.action === "imbalance");
    $("step-counter").textContent = frames.length <= 1
        ? "ШАГ 0 / 0"
        : `ШАГ ${frameIndex + 1} / ${frames.length}`;
    $("progress").max = frames.length - 1;
    $("progress").value = frameIndex;
    $("progress").style.setProperty(
        "--progress",
        `${frames.length > 1 ? (frameIndex / (frames.length - 1)) * 100 : 0}%`,
    );
    $("progress-label").textContent =
        frames.length <= 1 ? "0 из 0" : `${frameIndex + 1} из ${frames.length}`;
    const isTraversal = frames.some((step) => step.action === "visit");
    const sequence = frame.visited;
    $("sequence").replaceChildren();
    if (!sequence.length) {
        const hint = document.createElement("span");
        hint.className = "sequence-placeholder";
        hint.textContent = isTraversal
            ? "Первый узел появится на следующем шаге"
            : "Запусти обход, чтобы увидеть порядок узлов";
        $("sequence").append(hint);
    }
    for (const value of sequence) {
        const token = document.createElement("span");
        token.className = "sequence-value";
        token.textContent = value;
        $("sequence").append(token);
    }
    UpdateControls();
    if (rotation && frameIndex === frames.length - 1) {
        cleanupTimer = setTimeout(ClearRotation, (moving ? duration : 0) + 900);
    }
}

function ClearRotation() {
    $("tree").classList.add("settling");
    for (const group of nodeElements.values()) group.setAttribute("class", "node");
    for (const edge of edgeElements.values()) edge.element.setAttribute("class", "edge");
    $("action-title").textContent = "Готово";
    $("action-message").textContent = "";
    $("action-message").hidden = true;
    $("action-icon").textContent = "✓";
    $("action-icon").classList.remove("rotation");
    const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    cleanupTimer = setTimeout(
        () => {
            $("tree").classList.remove("settling");
        },
        reducedMotion ? 0 : 1600,
    );
}

function Pause() {
    clearTimeout(playTimer);
    playing = false;
    UpdateControls();
}

function ResetPlayback() {
    Pause();
    if (!frames.length) return;
    frames = [{
        nodes: frames[frames.length - 1].nodes,
        visited: [],
        action: "ready",
        message: "",
        current: null,
        related: null,
    }];
    layouts = [layouts[layouts.length - 1]];
    ShowFrame(0, false);
}

function ScheduleStep() {
    playTimer = setTimeout(
        () => {
            if (!playing) return;
            ShowFrame(frameIndex + 1);
            if (frameIndex === frames.length - 1) Pause();
            else ScheduleStep();
        },
        (frames[frameIndex].action.startsWith("rotate-") ? 2200 : 1050) / Number($("speed").value),
    );
}

function Play() {
    if (frames.length <= 1 || loading) return;
    if (playing) {
        Pause();
        return;
    }
    if (frameIndex === frames.length - 1) ShowFrame(0);
    playing = true;
    UpdateControls();
    ScheduleStep();
}

function ShowError(message) {
    clearTimeout(errorTimer);
    $("error").textContent = message;
    $("error").hidden = false;
    errorTimer = setTimeout(() => {
        $("error").hidden = true;
    }, 6500);
}

async function Request(action, value = 0) {
    if (loading) return;
    const requestTabVersion = tabVersion;
    Pause();
    if (frames.length) ShowFrame(frames.length - 1, false);
    loading = true;
    UpdateControls();
    try {
        const response = await fetch("/api/command", {
            method: "POST",
            body: new URLSearchParams({ action, value }),
        });
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || "Не удалось выполнить команду.");
        LoadFrames(result.frames);
        loading = false;
        if (requestTabVersion !== tabVersion) ResetPlayback();
        else {
            ShowFrame(0, false);
            Play();
        }
    } catch (error) {
        ShowError(
            error instanceof TypeError
                ? "Нет связи с приложением. Запусти avl_visualizer и обнови страницу."
                : error.message,
        );
    } finally {
        loading = false;
        UpdateControls();
    }
}

function UpdateLegend(name) {
    const rotation = name === "rotations";
    $("canvas-legend").classList.toggle("rotation-legend", rotation);
    $("legend-primary").textContent = rotation ? "Текущая пара" : "Текущий узел";
    $("legend-secondary").textContent = rotation ? "Участник" : "Посещён";
    $("legend-third").textContent = rotation ? "Связь" : "Балансировка";
    $("legend-third").parentElement.hidden = name === "traversal";
}

function SelectTab(name) {
    const currentTab = document.querySelector('[data-tab][aria-selected="true"]');
    if (currentTab.dataset.tab === name) return;
    tabVersion++;
    document.querySelectorAll("[data-tab]").forEach((button) => {
        const selected = button.dataset.tab === name;
        button.setAttribute("aria-selected", selected);
        button.tabIndex = selected ? 0 : -1;
        $(button.getAttribute("aria-controls")).hidden = !selected;
    });
    $("result-row").hidden = name !== "traversal";
    UpdateLegend(name);
    ResetPlayback();
}

const tabs = [...document.querySelectorAll("[data-tab]")];
tabs.forEach((button, index) => {
    button.addEventListener("click", () => SelectTab(button.dataset.tab));
    button.addEventListener("keydown", (event) => {
        if (!["ArrowLeft", "ArrowRight", "Home", "End"].includes(event.code)) return;
        event.preventDefault();
        event.stopPropagation();
        const next =
            event.code === "Home"
                ? 0
                : event.code === "End"
                  ? tabs.length - 1
                  : (index + (event.code === "ArrowRight" ? 1 : -1) + tabs.length) % tabs.length;
        SelectTab(tabs[next].dataset.tab);
        tabs[next].focus();
    });
});

document.querySelectorAll("[data-operation]").forEach((button) =>
    button.addEventListener("click", () => {
        operation = button.dataset.operation;
        document.querySelectorAll("[data-operation]").forEach((item) => {
            item.classList.toggle("selected", item === button);
            item.setAttribute("aria-pressed", item === button);
        });
        $("operation-label").textContent = {
            add: "Добавить узел",
            remove: "Удалить узел",
            find: "Найти узел",
        }[operation];
    }),
);
$("operation-form").addEventListener("submit", (event) => {
    event.preventDefault();
    if ($("operation-form").reportValidity()) Request(operation, Number($("value").value));
});
$("run-traversal").addEventListener("click", () => Request("traverse", Number($("traversal").value)));
document
    .querySelectorAll("[data-preset]")
    .forEach((button) =>
        button.addEventListener("click", () => Request("preset", Number(button.dataset.preset))),
    );
$("random").addEventListener("click", () => Request("random", Math.floor(Math.random() * 1000000)));
$("clear").addEventListener("click", () => Request("clear"));
$("play").addEventListener("click", Play);
$("previous").addEventListener("click", () => {
    Pause();
    ShowFrame(frameIndex - 1);
});
$("next").addEventListener("click", () => {
    Pause();
    ShowFrame(frameIndex + 1);
});
$("progress").addEventListener("input", () => {
    Pause();
    ShowFrame(Number($("progress").value));
});
$("speed").addEventListener("change", () => {
    if (playing) {
        clearTimeout(playTimer);
        ScheduleStep();
    }
});
$("fit").addEventListener("click", FitTree);
$("zoom-in").addEventListener("click", () => Zoom(0.8));
$("zoom-out").addEventListener("click", () => Zoom(1.25));
$("tree").addEventListener(
    "wheel",
    (event) => {
        event.preventDefault();
        const bounds = $("tree").getBoundingClientRect();
        Zoom(
            Math.exp(event.deltaY * 0.001),
            (event.clientX - bounds.left) / bounds.width,
            (event.clientY - bounds.top) / bounds.height,
        );
    },
    { passive: false },
);
let drag;
$("tree").addEventListener("pointerdown", (event) => {
    if (event.button !== 0 || event.target.closest(".node")) return;
    drag = { x: event.clientX, y: event.clientY, cameraX: camera.x, cameraY: camera.y };
    $("tree").setPointerCapture(event.pointerId);
    $("tree").classList.add("dragging");
});
$("tree").addEventListener("pointermove", (event) => {
    if (!drag) return;
    camera.x = drag.cameraX - ((event.clientX - drag.x) * camera.width) / $("tree").clientWidth;
    camera.y = drag.cameraY - ((event.clientY - drag.y) * camera.height) / $("tree").clientHeight;
    UpdateCamera();
});
function EndDrag() {
    drag = null;
    $("tree").classList.remove("dragging");
}
$("tree").addEventListener("pointerup", EndDrag);
$("tree").addEventListener("pointercancel", EndDrag);
document.addEventListener("keydown", (event) => {
    if (
        ["INPUT", "SELECT", "TEXTAREA"].includes(event.target.tagName) ||
        event.altKey ||
        event.ctrlKey ||
        event.metaKey ||
        loading
    )
        return;
    if (event.code === "Space" && event.target.tagName === "BUTTON") return;
    if (event.code === "Space") {
        event.preventDefault();
        Play();
    }
    if (event.code === "ArrowLeft") {
        event.preventDefault();
        Pause();
        ShowFrame(frameIndex - 1);
    }
    if (event.code === "ArrowRight") {
        event.preventDefault();
        Pause();
        ShowFrame(frameIndex + 1);
    }
});
new ResizeObserver(FitTree).observe($("tree-stage"));

async function Initialize() {
    const requestTabVersion = tabVersion;
    loading = true;
    UpdateControls();
    try {
        const response = await fetch("/api/tree");
        if (!response.ok) throw new Error("Сервер не смог передать дерево.");
        const result = await response.json();
        LoadFrames(result.frames);
        if (requestTabVersion !== tabVersion) ResetPlayback();
        else ShowFrame(frames.length - 1, false);
    } catch (error) {
        $("action-message").textContent = "Запусти avl_visualizer, затем обнови эту страницу.";
        ShowError(error instanceof TypeError ? "Нет связи с приложением." : error.message);
    } finally {
        loading = false;
        UpdateControls();
    }
}
Initialize();
