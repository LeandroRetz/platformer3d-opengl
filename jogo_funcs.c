/* ============================================================================
 *  jogo_funcs.c  -  TODA a logica do Platformer 3D.
 *
 *  Esqueleto inicial: globais definidos e funcoes em stub. As implementacoes
 *  sao preenchidas etapa a etapa (geracao de nivel, fisica, camera, render,
 *  estados de jogo).
 * ========================================================================== */
#include "main.h"

/* ----------------------------------------------------------------------------
 *  GLOBAIS
 * -------------------------------------------------------------------------- */
Jogador    g_jogador;
Plataforma g_nivel[NUM_PLATAFORMAS];
Estrela    g_estrelas[NUM_ESTRELAS];
Camera     g_camera;

int          g_estado            = ESTADO_MENU;
int          g_pontos            = 0;
int          g_vidas             = VIDAS_INICIAIS;
int          g_total_estrelas    = 0;
int          g_estrelas_coletadas = 0;
int          g_largura           = 1000;
int          g_altura            = 700;
int          g_reiniciando       = 0;
int          g_tempo_reinicio    = 0;
float        g_dificuldade       = 1.0f;
float        g_tempo             = 0.0f;
unsigned int g_semente           = 1u;

int          g_coyote            = 0;
int          g_buffer_pulo       = 0;
int          g_plat_suporte      = -1;

int          g_teclas[256];
int          g_arrastando        = 0;
int          g_posicao_x         = 0;
int          g_posicao_y         = 0;

/* ----------------------------------------------------------------------------
 *  INICIALIZACAO
 * -------------------------------------------------------------------------- */
void gInicializa(void) { /*STUB_INICIALIZA*/ }

void gInicializaJogo(void) { /*STUB_INICIALIZAJOGO*/ }

/* Gera o nivel de forma deterministica a partir de uma semente.
 * - plataforma 0: base 6x6 segura na origem
 * - plataformas 1..N-2: cursor avanca em X (gap 3..5), varia Y (-1..+3) e
 *   desvia Z (-3..+3); tipo sorteado (estatica / movel / fragil)
 * - plataforma N-1: meta 4x4 dourada
 * - ~25% das plataformas ganham uma estrela logo acima. */
void gInicializaNivel(unsigned int semente)
{
    int i;
    float cx, cy, cz, prev_right;

    srand(semente);
    g_total_estrelas = 0;
    /* marca todas as estrelas como "coletadas" => so as posicionadas aparecem */
    for (i = 0; i < NUM_ESTRELAS; i++) g_estrelas[i].coletada = 1;

    /* ---- plataforma inicial: 6x6, segura, na origem ---- */
    cx = 0.0f; cy = 0.0f; cz = 0.0f;
    g_nivel[0].x = 0.0f; g_nivel[0].y = 0.0f; g_nivel[0].z = 0.0f;
    g_nivel[0].largura = 6.0f; g_nivel[0].profundidade = 6.0f;
    g_nivel[0].tipo = 0; g_nivel[0].tem_estrela = 0; g_nivel[0].timer_fragil = -1.0f;
    g_nivel[0].base_x = 0.0f; g_nivel[0].base_z = 0.0f; g_nivel[0].fase = 0.0f;
    g_nivel[0].delta_x = 0.0f; g_nivel[0].delta_z = 0.0f;
    g_nivel[0].ativa = 1; g_nivel[0].eh_meta = 0;
    prev_right = 3.0f; /* borda direita da inicial (metade de 6) */

    /* ---- plataformas intermediarias ---- */
    for (i = 1; i < NUM_PLATAFORMAS - 1; i++) {
        Plataforma *p = &g_nivel[i];
        float w   = 2.2f + (rand() % 180) / 100.0f;   /* 2.2 .. 4.0 */
        float d   = 2.2f + (rand() % 180) / 100.0f;
        float gap = 3.0f + (rand() % 201) / 100.0f;   /* 3.0 .. 5.0 */
        int   dy  = (rand() % 5) - 1;                 /* -1 .. +3   */
        int   dz  = (rand() % 7) - 3;                 /* -3 .. +3   */
        int   r;

        cx  = prev_right + gap + w * 0.5f;            /* garante o gap entre bordas */
        prev_right = cx + w * 0.5f;
        cy += (float)dy;
        if (cy < -2.0f)  cy = -2.0f;
        if (cy > 14.0f)  cy = 14.0f;
        cz += (float)dz;
        if (cz < -12.0f) cz = -12.0f;
        if (cz > 12.0f)  cz = 12.0f;

        p->x = cx; p->y = cy; p->z = cz;
        p->largura = w; p->profundidade = d;
        p->base_x = cx; p->base_z = cz;
        p->fase = (rand() % 628) / 100.0f;            /* 0 .. ~2pi */
        p->delta_x = 0.0f; p->delta_z = 0.0f;
        p->ativa = 1; p->eh_meta = 0;
        p->timer_fragil = -1.0f;

        /* as 2 primeiras sempre estaticas (saida justa) */
        if (i <= 2) {
            p->tipo = 0;
        } else {
            r = rand() % 100;
            if      (r < 20) p->tipo = 1;             /* movel  */
            else if (r < 38) p->tipo = 2;             /* fragil */
            else             p->tipo = 0;             /* estatica */
        }

        /* ~25% com estrela acima da plataforma */
        p->tem_estrela = 0;
        if ((rand() % 100) < 26 && g_total_estrelas < NUM_ESTRELAS) {
            p->tem_estrela = 1;
            g_estrelas[g_total_estrelas].x = cx;
            g_estrelas[g_total_estrelas].y = cy + 1.3f;
            g_estrelas[g_total_estrelas].z = cz;
            g_estrelas[g_total_estrelas].coletada = 0;
            g_total_estrelas++;
        }
    }

    /* ---- plataforma final: 4x4 dourada (meta) ---- */
    {
        Plataforma *p = &g_nivel[NUM_PLATAFORMAS - 1];
        cx = prev_right + 4.0f + 2.0f;                /* gap + metade de 4 */
        p->x = cx; p->y = cy; p->z = cz;
        p->largura = 4.0f; p->profundidade = 4.0f;
        p->tipo = 0; p->tem_estrela = 0; p->timer_fragil = -1.0f;
        p->base_x = cx; p->base_z = cz; p->fase = 0.0f;
        p->delta_x = 0.0f; p->delta_z = 0.0f;
        p->ativa = 1; p->eh_meta = 1;
    }
}

/* ----------------------------------------------------------------------------
 *  FISICA / COLISAO
 * -------------------------------------------------------------------------- */
/* Consulta de chao: retorna o Y do topo da plataforma sob (x,z) que serve
 * de apoio para o jogador, ou -999 se nao houver nenhuma.
 * Considera apenas plataformas cujo topo esteja no nivel dos pes ou abaixo
 * (comportamento one-way: pousa-se vindo de cima). Tambem registra o indice
 * da plataforma escolhida em g_plat_suporte (usado p/ carregar moveis/fragil). */
GLfloat gChaoEm(GLfloat x, GLfloat z)
{
    int i, melhor = -1;
    GLfloat topo_melhor = -999.0f;
    const GLfloat tol = 0.05f;   /* tolerancia p/ "estar em pe" no topo */

    for (i = 0; i < NUM_PLATAFORMAS; i++) {
        Plataforma *p = &g_nivel[i];
        GLfloat hx, hz;
        if (!p->ativa) continue;
        hx = p->largura * 0.5f;
        hz = p->profundidade * 0.5f;
        if (x < p->x - hx || x > p->x + hx) continue;
        if (z < p->z - hz || z > p->z + hz) continue;
        if (p->y > g_jogador.y + tol) continue;      /* esta acima dos pes */
        if (p->y > topo_melhor) { topo_melhor = p->y; melhor = i; }
    }
    g_plat_suporte = melhor;
    return (melhor >= 0) ? topo_melhor : -999.0f;
}

/* Teste de "parede": 1 se o corpo do jogador sobrepoe a lateral da caixa da
 * plataforma. Retorna 0 quando o jogador esta em pe no topo (sem sobreposicao
 * vertical de corpo) — esses casos sao tratados como chao, nao como parede. */
int gColide(Plataforma *p)
{
    GLfloat px0, px1, pz0, pz1, ptop, pbot, jbot, jtop;
    if (!p->ativa) return 0;
    px0 = p->x - p->largura * 0.5f - RAIO_JOGADOR;
    px1 = p->x + p->largura * 0.5f + RAIO_JOGADOR;
    pz0 = p->z - p->profundidade * 0.5f - RAIO_JOGADOR;
    pz1 = p->z + p->profundidade * 0.5f + RAIO_JOGADOR;
    ptop = p->y;
    pbot = p->y - ESPESSURA_PLAT;
    jbot = g_jogador.y;
    jtop = g_jogador.y + ALTURA_JOGADOR;
    if (jtop <= pbot + 0.05f || jbot >= ptop - 0.05f) return 0;
    if (g_jogador.x <= px0 || g_jogador.x >= px1) return 0;
    if (g_jogador.z <= pz0 || g_jogador.z >= pz1) return 0;
    return 1;
}

/* Integra a posicao do jogador e resolve as colisoes:
 *  - horizontal: AABB, empurrando pelo eixo de menor penetracao
 *  - vertical:  via gChaoEm (pouso vindo de cima)
 *  - coleta de estrelas por proximidade. */
void gVerificaColisoes(void)
{
    int i;
    GLfloat old_y, new_y, chao;

    /* ---- horizontal: integra X/Z e resolve colisao lateral (AABB) ---- */
    g_jogador.x += g_jogador.vel_x * DT;
    g_jogador.z += g_jogador.vel_z * DT;

    for (i = 0; i < NUM_PLATAFORMAS; i++) {
        Plataforma *p = &g_nivel[i];
        GLfloat px0, px1, pz0, pz1;
        GLfloat penXe, penXd, penZf, penZt, minX, minZ;
        if (!gColide(p)) continue;

        px0 = p->x - p->largura * 0.5f - RAIO_JOGADOR;
        px1 = p->x + p->largura * 0.5f + RAIO_JOGADOR;
        pz0 = p->z - p->profundidade * 0.5f - RAIO_JOGADOR;
        pz1 = p->z + p->profundidade * 0.5f + RAIO_JOGADOR;

        penXe = g_jogador.x - px0;   /* empurrar p/ -X */
        penXd = px1 - g_jogador.x;   /* empurrar p/ +X */
        penZf = g_jogador.z - pz0;   /* empurrar p/ -Z */
        penZt = pz1 - g_jogador.z;   /* empurrar p/ +Z */
        minX  = (penXe < penXd) ? penXe : penXd;
        minZ  = (penZf < penZt) ? penZf : penZt;

        if (minX < minZ) {
            g_jogador.x = (penXe < penXd) ? px0 : px1;
            g_jogador.vel_x = 0.0f;
        } else {
            g_jogador.z = (penZf < penZt) ? pz0 : pz1;
            g_jogador.vel_z = 0.0f;
        }
    }

    /* ---- vertical: gravidade ja aplicada em vel_y; pouso via gChaoEm ---- */
    old_y = g_jogador.y;                        /* y antes de mover            */
    new_y = g_jogador.y + g_jogador.vel_y * DT;
    chao  = gChaoEm(g_jogador.x, g_jogador.z);  /* topo sob os pes (e suporte) */

    g_jogador.no_chao = 0;
    if (chao > -900.0f && g_jogador.vel_y <= 0.0f &&
        new_y <= chao && old_y >= chao - 0.5f) {
        /* pousou em cima da plataforma */
        g_jogador.y = chao;
        g_jogador.vel_y = 0.0f;
        g_jogador.no_chao = 1;

        if (g_plat_suporte >= 0) {
            Plataforma *p = &g_nivel[g_plat_suporte];
            if (p->tipo == 2 && p->timer_fragil < 0.0f)
                p->timer_fragil = FRAG_TEMPO;   /* aciona a fragil ao pisar */
            if (p->eh_meta && g_estado == ESTADO_JOGANDO) {
                g_estado = ESTADO_VITORIA;
                g_pontos += PONTOS_VITORIA;
            }
        }
    } else {
        g_jogador.y = new_y;
    }

    /* ---- coleta de estrelas por proximidade ---- */
    for (i = 0; i < g_total_estrelas; i++) {
        Estrela *e = &g_estrelas[i];
        GLfloat dx, dy, dz;
        if (e->coletada) continue;
        dx = e->x - g_jogador.x;
        dy = e->y - (g_jogador.y + 0.6f);
        dz = e->z - g_jogador.z;
        if (dx * dx + dy * dy + dz * dz < 1.0f) {
            e->coletada = 1;
            g_estrelas_coletadas++;
            g_pontos += 100;
        }
    }
}

/* Loop fisico: roda a ~60 Hz com passo fixo (DT). */
void gTimer(int valor)
{
    int i;
    (void)valor;

    g_tempo += DT;

    if (g_estado == ESTADO_JOGANDO) {
        /* --- 1. plataformas: moveis oscilam, frageis contam o tempo --- */
        for (i = 0; i < NUM_PLATAFORMAS; i++) {
            Plataforma *p = &g_nivel[i];
            if (!p->ativa) continue;
            if (p->tipo == 1) {
                float novo = p->base_z + sinf(g_tempo * 1.5f + p->fase) * 1.6f;
                p->delta_z = novo - p->z;   /* delta usado p/ carregar o jogador */
                p->delta_x = 0.0f;
                p->z = novo;
            } else {
                p->delta_x = 0.0f;
                p->delta_z = 0.0f;
            }
            if (p->tipo == 2 && p->timer_fragil >= 0.0f) {
                p->timer_fragil -= DT;
                if (p->timer_fragil <= 0.0f) p->ativa = 0;
            }
        }

        /* carrega o jogador junto da plataforma movel onde esta apoiado */
        if (g_plat_suporte >= 0 && g_jogador.no_chao &&
            g_nivel[g_plat_suporte].ativa && g_nivel[g_plat_suporte].tipo == 1) {
            g_jogador.x += g_nivel[g_plat_suporte].delta_x;
            g_jogador.z += g_nivel[g_plat_suporte].delta_z;
        }

        if (g_reiniciando) {
            /* --- delay de reinicio: respawna ou encerra em game over --- */
            g_tempo_reinicio++;
            if (g_tempo_reinicio >= TICKS_REINICIO) {
                g_reiniciando = 0;
                if (g_vidas <= 0) {
                    g_estado = ESTADO_GAME_OVER;
                } else {
                    g_jogador.x = 0.0f; g_jogador.z = 0.0f;
                    g_jogador.y = g_nivel[0].y + 0.05f;
                    g_jogador.vel_x = g_jogador.vel_y = g_jogador.vel_z = 0.0f;
                    g_jogador.no_chao = 1; g_jogador.vivo = 1;
                    g_coyote = 0; g_buffer_pulo = 0;
                }
            }
        } else {
            /* --- 2. entrada -> velocidade alvo, relativa ao yaw da camera --- */
            float yawR = g_camera.yaw * GRAUS_PARA_RAD;
            float fx = -sinf(yawR), fz = -cosf(yawR); /* "frente" da camera no plano XZ */
            float rx = -fz,         rz =  fx;         /* "direita" da camera            */
            float mvx = 0.0f, mvz = 0.0f, len;
            float alvo_vx = 0.0f, alvo_vz = 0.0f, resp;

            if (g_teclas['w']) { mvx += fx; mvz += fz; }
            if (g_teclas['s']) { mvx -= fx; mvz -= fz; }
            if (g_teclas['d']) { mvx += rx; mvz += rz; }
            if (g_teclas['a']) { mvx -= rx; mvz -= rz; }

            len = sqrtf(mvx * mvx + mvz * mvz);
            if (len > 0.001f) {
                mvx /= len; mvz /= len;
                alvo_vx = mvx * VEL_MOVIMENTO;
                alvo_vz = mvz * VEL_MOVIMENTO;
                g_jogador.yaw = atan2f(mvx, mvz) * RAD_PARA_GRAUS; /* corpo encara o movimento */
                g_jogador.frame_passo++;
            }
            /* lerp p/ a velocidade alvo: responde mais rapido no chao que no ar */
            resp = g_jogador.no_chao ? DRAG_CHAO : DRAG_AR;
            g_jogador.vel_x += (alvo_vx - g_jogador.vel_x) * resp;
            g_jogador.vel_z += (alvo_vz - g_jogador.vel_z) * resp;

            /* --- 3. gravidade --- */
            g_jogador.vel_y += GRAVIDADE * DT;

            /* --- 4. coyote time + jump buffer ---
             * coyote: alguns ticks apos sair da borda ainda permitem pular.
             * buffer: pulo pressionado um pouco antes de tocar o chao "espera". */
            if (g_jogador.no_chao)      g_coyote = FRAMES_COYOTE;
            else if (g_coyote > 0)      g_coyote--;
            if (g_buffer_pulo > 0)      g_buffer_pulo--;

            if (g_buffer_pulo > 0 && (g_jogador.no_chao || g_coyote > 0)) {
                g_jogador.vel_y   = VEL_PULO;
                g_jogador.no_chao = 0;
                g_coyote      = 0;
                g_buffer_pulo = 0;
            }

            /* --- 5. integra + colide --- */
            gVerificaColisoes();

            /* --- 6. caiu no vazio --- */
            if (g_jogador.y < QUEDA_LIMITE) {
                g_vidas--;
                g_jogador.vivo   = 0;
                g_reiniciando    = 1;
                g_tempo_reinicio = 0;
            }
        }
    }

    gAtualizaCamera();
    glutPostRedisplay();
    glutTimerFunc(16, gTimer, 0);
}

/* ----------------------------------------------------------------------------
 *  CAMERA
 * -------------------------------------------------------------------------- */
/* A camera orbital persegue o jogador suavemente (lerp por tick).
 * yaw/pitch/distancia sao controlados pelo mouse; a posicao do olho e
 * recalculada a partir do alvo em gDesenha. */
void gAtualizaCamera(void)
{
    Vec3 atual = { g_camera.alvo_x, g_camera.alvo_y, g_camera.alvo_z };
    Vec3 dest  = { g_jogador.x, g_jogador.y + 1.0f, g_jogador.z };
    Vec3 novo  = vec3Lerp(atual, dest, 0.08f);   /* fator de seguimento */
    g_camera.alvo_x = novo.x;
    g_camera.alvo_y = novo.y;
    g_camera.alvo_z = novo.z;
}

/* ----------------------------------------------------------------------------
 *  DESENHO
 * -------------------------------------------------------------------------- */
void gDesenha(void)
{
    /*STUB_DESENHA*/
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glutSwapBuffers();
}

void gDesenhaFundo(void) { /*STUB_FUNDO*/ }

void gDesenhaCeu(void) { /*STUB_CEU*/ }

void gDesenhaPlataforma(Plataforma *p) { (void)p; /*STUB_PLATAFORMA*/ }

void gDesenhaJogador(void) { /*STUB_JOGADOR*/ }

void gDesenhaEstrela(Estrela *e) { (void)e; /*STUB_ESTRELA*/ }

void gDesenhaHUD(void) { /*STUB_HUD*/ }

void gDesenhaMensagem(GLfloat x, GLfloat y, const char *texto)
{ (void)x; (void)y; (void)texto; /*STUB_MENSAGEM*/ }

/* ----------------------------------------------------------------------------
 *  ENTRADA / JANELA
 * -------------------------------------------------------------------------- */
void gTeclado(unsigned char tecla, int x, int y)
{ (void)tecla; (void)x; (void)y; /*STUB_TECLADO*/ }

void gTeclaSolta(unsigned char tecla, int x, int y)
{ (void)tecla; (void)x; (void)y; /*STUB_TECLASOLTA*/ }

/* Botao direito inicia/encerra o drag de orbita; roda do mouse faz zoom. */
void gMouseBotao(int botao, int estado, int x, int y)
{
    if (botao == GLUT_RIGHT_BUTTON) {
        if (estado == GLUT_DOWN) {
            g_arrastando = 1;
            g_posicao_x  = x;
            g_posicao_y  = y;
        } else {
            g_arrastando = 0;
        }
    }
    /* roda do mouse (freeglut: botoes 3 = cima, 4 = baixo) ajusta a distancia */
    if (botao == 3 && estado == GLUT_DOWN) g_camera.distancia -= 1.0f;
    if (botao == 4 && estado == GLUT_DOWN) g_camera.distancia += 1.0f;
    if (g_camera.distancia < 8.0f)  g_camera.distancia = 8.0f;
    if (g_camera.distancia > 16.0f) g_camera.distancia = 16.0f;
}

/* Arrasto com o botao direito gira a camera; pitch fica preso entre -60 e -10. */
void gMouseMov(int x, int y)
{
    if (g_arrastando) {
        int dx = x - g_posicao_x;
        int dy = y - g_posicao_y;
        g_camera.yaw   += dx * 0.30f;
        g_camera.pitch -= dy * 0.30f;
        if (g_camera.pitch < -60.0f) g_camera.pitch = -60.0f;
        if (g_camera.pitch > -10.0f) g_camera.pitch = -10.0f;
        g_posicao_x = x;
        g_posicao_y = y;
    }
}

void gRedimensiona(int largura, int altura)
{
    g_largura = largura;
    g_altura  = (altura > 0) ? altura : 1;
    glViewport(0, 0, g_largura, g_altura);
    /*STUB_REDIMENSIONA*/
}
