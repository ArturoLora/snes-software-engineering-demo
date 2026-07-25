--------------------------------------------------------------------------------
-- poc_read_memory.lua
--
-- Proof of Concept: verificar que BizHawk 2.11.1 puede leer el estado de la ROM
-- de Apotris SNES desde Lua, frame a frame.
--
-- NO es el harness. NO automatiza nada. Solo lee y reporta.
--
-- Direcciones derivadas de snes/apotris.sym (generado por wlalink) y del layout
-- de GameState en snes/source/game_state.h:
--
--   007e2805  pad0                        (u16, PVSnesLib pad state)
--   007e2807  tccs_source/main.asm_gs     (GameState gs)
--
--   GameState {
--     BoardState  board;   // u8[22][10]  -> offset   0 .. 219
--     ActivePiece piece;   // u8,u8,s8,s8 -> offset 220 .. 223
--     LinesToClear lines;  // u8[4], u8   -> offset 224 .. 228
--   }                                     // total 229 bytes
--
-- Verificacion del layout: el siguiente simbolo en el .sym es 007e28ec
-- (test_status) y 0x7e2807 + 229 = 0x7e28ec -> sin padding.
--
-- ADVERTENCIA (hallazgo G1, Story 4-8). Estas dos direcciones son CONSTANTES
-- PEGADAS A MANO, y eso es un defecto conocido, no una decision.
--
-- Cualquier Story que anada una variable en WRAM desplaza el layout y las
-- invalida. Cuando eso pasa, este script lee memoria arbitraria y el harness
-- SIGUE DEVOLVIENDO PASS: sus criterios (>=2 lecturas, >=180 frames, "estado de
-- juego visto") se satisfacen igual de bien con basura. Es para V1 lo que el
-- hallazgo H4 era para V0: un nivel exigido que devuelve exito sin medir nada.
--
-- Ya ocurrio una vez: la Story 4-8 anadio un buffer de 2048 bytes que se asigno
-- en $7E2000 y empujo pad0 de $7E2000 a $7E2805 y gs de $7E2002 a $7E2807. La
-- unica senal fue que las cifras del harness cambiaron de 14 lecturas / 203
-- frames a 6 / 186.
--
-- El arreglo real es resolver ambos simbolos desde apotris.sym en tiempo de
-- ejecucion — los dos existen ahi con nombre. Queda registrado como G1 y
-- pendiente de su propia Story de clase `herramientas`.
--
-- Mientras tanto: si el log muestra valores congelados o incoherentes con el
-- juego, comprobar ESTO ANTES que nada:
--     grep -iE "^007e.* (pad0|main.asm_gs)" snes/apotris.sym
--------------------------------------------------------------------------------

local BUS_PAD0 = 0x7E2805
local BUS_GS   = 0x7E2807

local OFF_PIECE_TYPE     = 220
local OFF_PIECE_ROTATION = 221
local OFF_PIECE_X        = 222
local OFF_PIECE_Y        = 223
local OFF_LINES_COUNT    = 228

local HEARTBEAT_FRAMES = 60

-- Copia de la salida a archivo, para dejar evidencia revisable tras la corrida.
-- Se escribe junto al script. Si no puede abrirse, el PoC sigue sin archivo.
local LOG_PATH = "/home/arturo/Projects/apotris-snes/tools/lua/poc_read_memory.log"
-- pcall: si esta build de BizHawk restringe la libreria `io`, el PoC sigue
-- funcionando solo contra la consola.
local ok_log, log_file = pcall(function() return io.open(LOG_PATH, "w") end)
if not ok_log then log_file = nil end

local function emit(line)
	console.log(line)
	if log_file then
		log_file:write(line, "\n")
		log_file:flush()
	end
end

--------------------------------------------------------------------------------
-- Seleccion de dominio de memoria
--
-- El script no asume el dominio: enumera los dominios reales del core cargado
-- y elige. En el core SNES de BizHawk el dominio "WRAM" son los 128 KiB de RAM
-- de trabajo, donde el offset 0 corresponde al bus $7E0000. Si "WRAM" no
-- existe, cae a "System Bus", donde la direccion es la direccion de bus tal
-- cual.
--------------------------------------------------------------------------------

-- getmemorydomainlist() devuelve una tabla en BizHawk 2.11.1, aunque los .d.lua
-- de la propia build documenten "un string delimitado por saltos de linea".
-- Se aceptan ambas formas.
local function to_domain_list(v)
	local out = {}
	if type(v) == "table" then
		for _, d in pairs(v) do
			if type(d) == "string" and d ~= "" then
				out[#out + 1] = d
			end
		end
	elseif type(v) == "string" then
		for line in string.gmatch(v, "[^\r\n]+") do
			out[#out + 1] = line
		end
	end
	return out
end

local function pick_domain()
	local domains = to_domain_list(memory.getmemorydomainlist())

	emit("Dominios de memoria del core actual:")
	for _, d in ipairs(domains) do
		emit(string.format("  - %s (%d bytes)", d, memory.getmemorydomainsize(d)))
	end

	for _, d in ipairs(domains) do
		if d == "WRAM" then
			-- WRAM offset 0 == bus $7E0000
			return d, 0x7E0000
		end
	end

	for _, d in ipairs(domains) do
		if d == "System Bus" then
			return d, 0
		end
	end

	error("No se encontro dominio 'WRAM' ni 'System Bus'. Core cargado: " .. tostring(emu.getsystemid()))
end

local domain, bus_base = pick_domain()
memory.usememorydomain(domain)

local ADDR_PAD0 = BUS_PAD0 - bus_base
local ADDR_GS   = BUS_GS - bus_base

emit("")
emit(string.format("Sistema ....... %s", emu.getsystemid()))
emit(string.format("Dominio usado . %s", memory.getcurrentmemorydomain()))
emit(string.format("Tamano dominio  %d bytes", memory.getcurrentmemorydomainsize()))
emit(string.format("pad0 en bus $%06X -> offset 0x%X del dominio", BUS_PAD0, ADDR_PAD0))
emit(string.format("gs   en bus $%06X -> offset 0x%X del dominio", BUS_GS, ADDR_GS))
emit("")
emit("Leyendo. Mueve la pieza con el pad para ver cambiar los valores.")
emit("--------------------------------------------------------------")

--------------------------------------------------------------------------------
-- Lectura por frame
--------------------------------------------------------------------------------

local function read_state()
	return {
		pad      = memory.read_u16_le(ADDR_PAD0),
		ptype    = memory.read_u8(ADDR_GS + OFF_PIECE_TYPE),
		rotation = memory.read_u8(ADDR_GS + OFF_PIECE_ROTATION),
		x        = memory.read_s8(ADDR_GS + OFF_PIECE_X),
		y        = memory.read_s8(ADDR_GS + OFF_PIECE_Y),
		lines    = memory.read_u8(ADDR_GS + OFF_LINES_COUNT),
	}
end

local function fmt(s)
	return string.format(
		"f=%-7d pad=%04X  piece[type=%d rot=%d x=%d y=%d]  lines=%d",
		emu.framecount(), s.pad, s.ptype, s.rotation, s.x, s.y, s.lines)
end

local function same(a, b)
	return a.pad == b.pad
		and a.ptype == b.ptype
		and a.rotation == b.rotation
		and a.x == b.x
		and a.y == b.y
		and a.lines == b.lines
end

local prev = nil
local frames_since_print = 0

while true do
	local cur = read_state()

	-- Overlay en pantalla: prueba visual de que la lectura ocurre cada frame.
	gui.text(4, 4, fmt(cur))

	-- Consola: imprime en cada cambio de estado, mas un latido periodico para
	-- demostrar que el bucle sigue avanzando cuando nada cambia.
	frames_since_print = frames_since_print + 1
	if prev == nil then
		emit("INIT   " .. fmt(cur))
		frames_since_print = 0
	elseif not same(prev, cur) then
		emit("CHANGE " .. fmt(cur))
		frames_since_print = 0
	elseif frames_since_print >= HEARTBEAT_FRAMES then
		emit("IDLE   " .. fmt(cur))
		frames_since_print = 0
	end

	prev = cur
	emu.frameadvance()
end
