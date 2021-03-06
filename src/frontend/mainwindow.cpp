#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    editwindow = new EditWindow(0, this);
    srcimglabel = new SrcImgLabel(ui->src_bottom_info);
    desimglabel = new DesImgLabel(ui->des_bottom_info);
    this->adjust_widget();
    connect(this->ui->actiongo_to_switch, SIGNAL(triggered()), this, SLOT(switch_to_edit()));

    QButtonGroup *btgroup = new QButtonGroup(this);
    btgroup->addButton(ui->poly);
    btgroup->addButton(ui->rect);

    connect(ui->poly, SIGNAL(clicked()), this, SLOT(on_poly_clicked()));
    connect(ui->rect, SIGNAL(clicked()), this, SLOT(on_rect_clicked()));

    //this->ui->fusion_way->addItem("POISSON_OPENCV_NORMAL");
    //this->ui->fusion_way->addItem("POISSON_OPENCV_MIXED");
    //this->ui->fusion_way->addItem("POISSON_OWN_RECT");
    //this->ui->fusion_way->addItem("POISSON_OWN_POLY");
    this->ui->fusion_way->addItem("POISSON_FR");
    this->ui->fusion_way->addItem("POISSON_DRAG");
    //this->ui->fusion_way->addItem("-");
    //this->ui->fusion_way->addItem("POISSON_Fast");
    connect(this->ui->fusion_way, SIGNAL(currentIndexChanged(int)), this, SLOT(switch_fusion_way()));
    ui->fusion_way->setCurrentIndex(0);
    //cout << "init:" << ui->fusion_way->currentIndex();
    srcimglabel->poly = createPolygon();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_open_src_img_clicked(){
    srcimglabel->selectOver = false;
    srcimglabel->open_img(ui->src_img_area);
}

void MainWindow::on_open_des_img_clicked(){
    desimglabel->hasSubImg = false;
    desimglabel->open_img(ui->des_img_area);
}

void MainWindow::on_select_ok_clicked(){
    if(srcimglabel->selectOver == false) return;
    if(desimglabel->hasImg == false) return;
    desimglabel->SELECT_WAY = srcimglabel->SELECT_WAY;
    if(srcimglabel->SELECT_WAY == POLY){
        getPolygonRect(srcimglabel->poly,
                       &(srcimglabel->minx),
                       &(srcimglabel->miny),
                       &(srcimglabel->maxx),
                       &(srcimglabel->maxy));
        copyPolygon(srcimglabel->poly,&(desimglabel->poly));
        minPolygon(&(desimglabel->poly),srcimglabel->minx,srcimglabel->miny);
        print_poly(desimglabel->poly);
    }
    desimglabel->hasSubImg = true;
    desimglabel->subx = 0;
    desimglabel->suby = 0;
    int imgx = srcimglabel->minx - srcimglabel->img_anchor_x;
    int imgy = srcimglabel->miny - srcimglabel->img_anchor_y;
    int w = srcimglabel->maxx - srcimglabel->minx + 1;
    int h = srcimglabel->maxy - srcimglabel->miny + 1;
    desimglabel->subw = w;
    desimglabel->subh = h;
    desimglabel->subimagemask = NULL;
    desimglabel->subimagemaskscale = NULL;
    if(desimglabel->SELECT_WAY == POLY ){
        desimglabel->subimagemask = new QImage(w, h, srcimglabel->image->format());
        desimglabel->subimagemaskscale = new QImage(w, h, srcimglabel->image->format());
        getPolygonMask(desimglabel->poly,w,h,desimglabel->submask);
        for(int y = 0; y < h; y++){
            for(int x = 0; x < w; x++){
                if(desimglabel->submask[y][x]==1){
                    desimglabel->subimagemask->setPixel(x,y,qRgb(255,255,255));
                }
                else{
                    desimglabel->subimagemask->setPixel(x,y,qRgb(0,0,0));
                }
            }
        }
        *(desimglabel->subimagemaskscale) = desimglabel->subimagemask->scaled(w, h, Qt::IgnoreAspectRatio);
    }
    desimglabel->subimage = new QImage(w, h, srcimglabel->image->format());
    desimglabel->subimagescale = new QImage(w, h, srcimglabel->image->format());
    for(int y = imgy; y < imgy + h ; y++){
        QRgb *rowData = (QRgb*)srcimglabel->image->scanLine(y);
        for(int x = imgx; x < imgx + w; x++){
            desimglabel->subimage->setPixel(x-imgx,y-imgy,rowData[x]);
        }
    }
    *(desimglabel->subimagescale) = desimglabel->subimage->scaled(w, h, Qt::IgnoreAspectRatio);
    desimglabel->needDraw = true;
    desimglabel->update();
}

void MainWindow::on_poisson_clicked(){
    desimglabel->poisson();
}

void MainWindow::on_save_img_clicked(){
    if(!desimglabel || !desimglabel->hasImg) return;
    char ss[128];
    string fs1 = getPureName(srcimglabel->filename.toStdString());
    string fs2 = getPureName(desimglabel->filename.toStdString());
    const char* s1 = fs1.c_str();
    const char* s2 = fs2.c_str();
    sprintf(ss,"%s_%s.jpg",s1,s2);
    desimglabel->save_img(ss);
}

void MainWindow::on_backward_clicked(){
    desimglabel->forward_backward(-1);
}

void MainWindow::on_forward_clicked(){
    desimglabel->forward_backward(1);
}

void MainWindow::on_clear_roi_clicked(){
    if(desimglabel->hasImg && desimglabel->hasSubImg){
        desimglabel->hasSubImg = false;
        desimglabel->setPixmap(QPixmap::fromImage(*(desimglabel->image)));
    }
}

void MainWindow::adjust_widget(){
    QDesktopWidget* desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->availableGeometry();
    this->resize(QSize(screenRect.width(), screenRect.height()));
    int w = this->width();
    int h = this->height();
    int MAX_X = std::max((w - 20 * 2)/2, srcimglabel->MAX_X);
    int MAX_Y = std::max((h - 100), srcimglabel->MAX_Y);
    srcimglabel->MAX_X = MAX_X;
    srcimglabel->MAX_Y = MAX_Y;
    desimglabel->MAX_X = MAX_X;
    desimglabel->MAX_Y = MAX_Y;
    //std::cout << w << " " << h << " " << MAX_X << " " << MAX_Y << std::endl;
    this->ui->src_img_area->setGeometry(20, 40, MAX_X, MAX_Y);
    this->ui->des_img_area->setGeometry(20 + MAX_X, 40, MAX_X, MAX_Y);
    this->ui->src_bottom_info->setGeometry(20, 40 + MAX_Y, MAX_X, 20);
    this->ui->des_bottom_info->setGeometry(20 + MAX_X, 40 + MAX_Y, MAX_X, 20);

    this->ui->fusion_way->setGeometry(20 + MAX_X - 260, 10, 250, 25);
    int bw = 90, bh = 25, bww = 100;
    this->ui->open_des_img->setGeometry(20 + MAX_X + bww * 0, 10, bw, bh);
    this->ui->poisson->setGeometry(20 + MAX_X + bww * 1, 10, bw, bh);
    this->ui->backward->setGeometry(20 + MAX_X + bww * 2, 10, bw, bh);
    this->ui->forward->setGeometry(20 + MAX_X + bww * 3, 10, bw, bh);
    this->ui->clear_roi->setGeometry(20 + MAX_X + bww * 4, 10, bw, bh);
    this->ui->save_img->setGeometry(20 + MAX_X + bww * 5, 10, bw, bh);
}

void MainWindow::switch_to_edit(){
    this->hide();
    this->editwindow->show();
}

void MainWindow::on_rect_clicked(){
    srcimglabel->SELECT_WAY = RECT;
    srcimglabel->clear_select();
    ui->fusion_way->setCurrentIndex(0);
}

void MainWindow::on_poly_clicked(){
    srcimglabel->SELECT_WAY = POLY;
    srcimglabel->clear_select();
    ui->fusion_way->setCurrentIndex(0);
}

void MainWindow::switch_fusion_way(){
    int ind = ui->fusion_way->currentIndex();
    if(ind == 0) desimglabel->ALGO = POISSON_FR;
    else if(ind == 1) desimglabel->ALGO = POISSON_DRAG;
    //else if(ind == 2 && srcimglabel->SELECT_WAY == RECT) desimglabel->ALGO = POISSON_OWN_RECT;
    //else if(ind == 3 && srcimglabel->SELECT_WAY == POLY) desimglabel->ALGO = POISSON_OWN_POLY;
    //else if(ind == 4 && srcimglabel->SELECT_WAY == POLY) desimglabel->ALGO = POISSON_OWN_DRAG;
    //else if(ind == 5) desimglabel->ALGO = POISSON_FR;
    //else if(ind == 6) desimglabel->ALGO = POISSON_Fast;
    else desimglabel->ALGO = POISSON_OPENCV_NORMAL;
    // make sure that own_poly and own_drag not match with rect select way,
    // own_rect not match with poly select way
    //if(   (ind == 2 && srcimglabel->SELECT_WAY != RECT)
    //   || (ind == 3 && srcimglabel->SELECT_WAY != POLY)
    //  || (ind == 4 && srcimglabel->SELECT_WAY != POLY) )
    //    ui->fusion_way->setCurrentIndex(0);
    QByteArray ba(ui->fusion_way->currentText().toAscii());
    cout << "Change FUSION_WAY : " << ind << " : "<< ba.data() << endl;
}



