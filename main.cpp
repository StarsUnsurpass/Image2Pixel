#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFileDialog>
#include <QImage>
#include <QPixmap>
#include <QScrollArea>
#include <QStyle>
#include <QPainter>
#include <QStandardPaths>
#include <QWheelEvent>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

class PixelatorWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class Language {
        English,
        Chinese
    };

    PixelatorWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("image2pixel - Modern Edition");
        resize(1000, 700);

        // Apply a modern dark theme style
        setStyleSheet(R"(
            QMainWindow {
                background-color: #2b2b2b;
            }
            QLabel {
                color: #e0e0e0;
                font-family: "Segoe UI", sans-serif;
                font-size: 14px;
            }
            QPushButton {
                background-color: #3c3f41;
                color: white;
                border: 1px solid #555;
                border-radius: 5px;
                padding: 8px 16px;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: #484b4d;
            }
            QPushButton:pressed {
                background-color: #505355;
            }
            QSlider::groove:horizontal {
                border: 1px solid #999999;
                height: 8px;
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);
                margin: 2px 0;
                border-radius: 4px;
            }
            QSlider::handle:horizontal {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #b4b4b4, stop:1 #8f8f8f);
                border: 1px solid #5c5c5c;
                width: 18px;
                margin: -2px 0;
                border-radius: 9px;
            }
        )");

        QWidget *centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // --- Menu Bar Setup ---
        QMenuBar *menuBar = new QMenuBar(this);
        setMenuBar(menuBar);

        // Settings Menu
        settingsMenu = menuBar->addMenu("Settings");
        langMenu = settingsMenu->addMenu("Language");
        
        QAction *actChinese = langMenu->addAction("中文 (Chinese)");
        QAction *actEnglish = langMenu->addAction("English");
        
        connect(actChinese, &QAction::triggered, [this](){
            currentLanguage = Language::Chinese;
            updateTexts();
        });
        connect(actEnglish, &QAction::triggered, [this](){
            currentLanguage = Language::English;
            updateTexts();
        });

        // Help Menu
        helpMenu = menuBar->addMenu("Help");
        aboutAction = helpMenu->addAction("About");
        connect(aboutAction, &QAction::triggered, this, &PixelatorWindow::showAboutDialog);

        // Top Toolbar Area
        QHBoxLayout *toolbarLayout = new QHBoxLayout();
        
        btnOpen = new QPushButton("Open Image");
        btnSave = new QPushButton("Save Image");
        btnSave->setEnabled(false);

        // Zoom Controls
        btnZoomIn = new QPushButton("+");
        btnZoomOut = new QPushButton("-");
        btnZoomIn->setFixedWidth(40);
        btnZoomOut->setFixedWidth(40);
        btnZoomIn->setToolTip("Zoom In (Ctrl + Wheel Up)");
        btnZoomOut->setToolTip("Zoom Out (Ctrl + Wheel Down)");

        lblBlockSize = new QLabel("Pixel Size: 10");
        sliderBlockSize = new QSlider(Qt::Horizontal);
        sliderBlockSize->setRange(1, 100);
        sliderBlockSize->setValue(10);
        sliderBlockSize->setFixedWidth(200);

        toolbarLayout->addWidget(btnOpen);
        toolbarLayout->addWidget(btnSave);
        toolbarLayout->addSpacing(20);
        zoomLabel = new QLabel("Zoom:");
        toolbarLayout->addWidget(zoomLabel);
        toolbarLayout->addWidget(btnZoomOut);
        toolbarLayout->addWidget(btnZoomIn);
        toolbarLayout->addStretch();
        toolbarLayout->addWidget(lblBlockSize);
        toolbarLayout->addWidget(sliderBlockSize);

        mainLayout->addLayout(toolbarLayout);

        // Image Display Area
        scrollArea = new QScrollArea;
        scrollArea->setBackgroundRole(QPalette::Dark);
        scrollArea->setWidgetResizable(false); // Important for zooming
        scrollArea->setAlignment(Qt::AlignCenter);
        scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #1e1e1e; }");

        imageLabel = new QLabel;
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setText("No image loaded.<br>Click <b>Open Image</b> to start.");
        imageLabel->setStyleSheet("color: #777;");
        imageLabel->setScaledContents(true); // Allow pixmap to scale with label size
        
        // Enable mouse tracking for smoother interaction if needed, 
        // but event filter is enough for wheel.
        imageLabel->installEventFilter(this); 

        scrollArea->setWidget(imageLabel);
        mainLayout->addWidget(scrollArea);

        // Status Bar
        statusLabel = new QLabel("Ready");
        statusLabel->setStyleSheet("color: #888; font-size: 11px;");
        mainLayout->addWidget(statusLabel);

        // Connections
        connect(btnOpen, &QPushButton::clicked, this, &PixelatorWindow::openImage);
        connect(btnSave, &QPushButton::clicked, this, &PixelatorWindow::saveImage);
        connect(btnZoomIn, &QPushButton::clicked, [this](){ scaleImage(1.25); });
        connect(btnZoomOut, &QPushButton::clicked, [this](){ scaleImage(0.8); });
        
        connect(sliderBlockSize, &QSlider::valueChanged, this, &PixelatorWindow::updatePixelation);
        connect(sliderBlockSize, &QSlider::valueChanged, [this](int val){
            if (currentLanguage == Language::Chinese)
                 lblBlockSize->setText(QString::fromUtf8("像素大小: %1").arg(val));
            else
                 lblBlockSize->setText(QString("Pixel Size: %1").arg(val));
        });

        // Initialize Texts
        updateTexts();
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == imageLabel && event->type() == QEvent::Wheel) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            if (wheelEvent->modifiers() & Qt::ControlModifier) {
                double factor = (wheelEvent->angleDelta().y() > 0) ? 1.25 : 0.8;
                scaleImage(factor);
                return true; // Event handled
            }
        }
        return QMainWindow::eventFilter(obj, event);
    }

private slots:
    void openImage() {
        QString title = (currentLanguage == Language::Chinese) ? QString::fromUtf8("打开图片") : "Open Image";
        QString filter = (currentLanguage == Language::Chinese) ? QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp)") : "Images (*.png *.jpg *.jpeg *.bmp)";
        
        QString fileName = QFileDialog::getOpenFileName(this, title, 
                                                        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), 
                                                        filter);
        if (!fileName.isEmpty()) {
            if (originalImage.load(fileName)) {
                // Convert to ARGB32 for easier pixel manipulation
                originalImage = originalImage.convertToFormat(QImage::Format_ARGB32);
                currentFilePath = fileName;
                btnSave->setEnabled(true);
                scaleFactor = 1.0; // Reset zoom
                updatePixelation();
                if (currentLanguage == Language::Chinese)
                    statusLabel->setText(QString::fromUtf8("已加载: %1 (100%)").arg(fileName));
                else
                    statusLabel->setText(QString("Loaded: %1 (100%)").arg(fileName));
            }
        }
    }

    void saveImage() {
        if (processedImage.isNull()) return;
        
        QString title = (currentLanguage == Language::Chinese) ? QString::fromUtf8("保存图片") : "Save Image";
        QString filter = (currentLanguage == Language::Chinese) ? QString::fromUtf8("PNG 图片 (*.png);;JPEG 图片 (*.jpg)") : "PNG Image (*.png);;JPEG Image (*.jpg)";

        QString fileName = QFileDialog::getSaveFileName(this, title, 
                                                        currentFilePath, 
                                                        filter);
        if (!fileName.isEmpty()) {
            if (processedImage.save(fileName)) {
                if (currentLanguage == Language::Chinese)
                    statusLabel->setText(QString::fromUtf8("已保存至: %1").arg(fileName));
                else
                    statusLabel->setText(QString("Saved to: %1").arg(fileName));
            } else {
                if (currentLanguage == Language::Chinese)
                    statusLabel->setText(QString::fromUtf8("保存图片失败。"));
                else
                    statusLabel->setText("Error saving image.");
            }
        }
    }

    void updatePixelation() {
        if (originalImage.isNull()) return;

        int blockSize = sliderBlockSize->value();
        if (blockSize <= 1) {
            processedImage = originalImage;
        } else {
            processedImage = pixelate(originalImage, blockSize);
        }

        updateImageDisplay();
    }

    void scaleImage(double factor) {
        if (processedImage.isNull()) return;
        
        scaleFactor *= factor;
        
        // Limit zoom
        if (scaleFactor < 0.1) scaleFactor = 0.1;
        if (scaleFactor > 5.0) scaleFactor = 5.0;

        updateImageDisplay();
        
        if (currentLanguage == Language::Chinese)
            statusLabel->setText(QString::fromUtf8("缩放: %1%").arg((int)(scaleFactor * 100)));
        else
            statusLabel->setText(QString("Zoom: %1%").arg((int)(scaleFactor * 100)));
    }

    void updateImageDisplay() {
        if (processedImage.isNull()) return;
        
        QSize newSize = scaleFactor * processedImage.size();
        imageLabel->resize(newSize);
        imageLabel->setPixmap(QPixmap::fromImage(processedImage));
    }

    void showAboutDialog() {
        QMessageBox aboutBox(this);
        aboutBox.setTextFormat(Qt::RichText);
        
        if (currentLanguage == Language::Chinese) {
            aboutBox.setWindowTitle(QString::fromUtf8("关于 Image2Pixel"));
            aboutBox.setText(
                QString::fromUtf8("<h3>Image2Pixel - 现代版</h3>"
                "<p>一款高性能的图片像素化工具。</p>"
                "<hr>"
                "<p><b>作者:</b> StarsUnsurpass</p>"
                "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>"
                "<p><b>项目地址:</b> <a href='https://github.com/StarsUnsurpass/Image2Pixel'>https://github.com/StarsUnsurpass/Image2Pixel</a></p>")
            );
        } else {
            aboutBox.setWindowTitle("About Image2Pixel");
            aboutBox.setText(
                "<h3>Image2Pixel - Modern Edition</h3>"
                "<p>A high-performance image pixelation tool.</p>"
                "<hr>"
                "<p><b>Author:</b> StarsUnsurpass</p>"
                "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>"
                "<p><b>Project:</b> <a href='https://github.com/StarsUnsurpass/Image2Pixel'>https://github.com/StarsUnsurpass/Image2Pixel</a></p>"
            );
        }
        aboutBox.setStandardButtons(QMessageBox::Ok);
        aboutBox.exec();
    }

private:
    QImage pixelate(const QImage &source, int blockSize) {
        QImage result = source.copy();
        int width = result.width();
        int height = result.height();

        // Direct memory access for speed (simplified for readability/safety)
        for (int y = 0; y < height; y += blockSize) {
            for (int x = 0; x < width; x += blockSize) {
                
                long r = 0, g = 0, b = 0, a = 0;
                int count = 0;

                // 1. Calculate Average
                for (int by = 0; by < blockSize; ++by) {
                    if (y + by >= height) break;
                    for (int bx = 0; bx < blockSize; ++bx) {
                        if (x + bx >= width) break;
                        
                        QColor pixelColor = source.pixelColor(x + bx, y + by);
                        r += pixelColor.red();
                        g += pixelColor.green();
                        b += pixelColor.blue();
                        a += pixelColor.alpha();
                        count++;
                    }
                }

                if (count == 0) continue;
                
                QColor avgColor(r/count, g/count, b/count, a/count);
                QRgb avgRgb = avgColor.rgba();

                // 2. Fill Block
                for (int by = 0; by < blockSize; ++by) {
                    int destY = y + by;
                    if (destY >= height) break;
                    
                    // Optimization: Get scanline pointer could be faster, but setPixel is safer
                    for (int bx = 0; bx < blockSize; ++bx) {
                        int destX = x + bx;
                        if (destX >= width) break;
                        
                        result.setPixel(destX, destY, avgRgb);
                    }
                }
            }
        }
        return result;
    }

    void updateTexts() {
        if (currentLanguage == Language::Chinese) {
            setWindowTitle(QString::fromUtf8("图片像素化工具 - 现代版"));
            btnOpen->setText(QString::fromUtf8("打开图片"));
            btnSave->setText(QString::fromUtf8("保存图片"));
            zoomLabel->setText(QString::fromUtf8("缩放:"));
            
            // Note: block size label is dynamic, will update on next value change or forced update
            lblBlockSize->setText(QString::fromUtf8("像素大小: %1").arg(sliderBlockSize->value()));
            
            helpMenu->setTitle(QString::fromUtf8("帮助"));
            settingsMenu->setTitle(QString::fromUtf8("设置"));
            langMenu->setTitle(QString::fromUtf8("语言"));
            aboutAction->setText(QString::fromUtf8("关于"));
            
            if (originalImage.isNull()) {
                imageLabel->setText(QString::fromUtf8("未加载图片。<br>点击 <b>打开图片</b> 开始。"));
            }
            if (statusLabel->text() == "Ready" || statusLabel->text() == "就绪") {
                statusLabel->setText(QString::fromUtf8("就绪"));
            }

        } else {
            setWindowTitle("Image2Pixel - Modern Edition");
            btnOpen->setText("Open Image");
            btnSave->setText("Save Image");
            zoomLabel->setText("Zoom:");
            
            lblBlockSize->setText(QString("Pixel Size: %1").arg(sliderBlockSize->value()));
            
            helpMenu->setTitle("Help");
            settingsMenu->setTitle("Settings");
            langMenu->setTitle("Language");
            aboutAction->setText("About");
            
            if (originalImage.isNull()) {
                imageLabel->setText("No image loaded.<br>Click <b>Open Image</b> to start.");
            }
            if (statusLabel->text() == "Ready" || statusLabel->text() == "就绪") {
                statusLabel->setText("Ready");
            }
        }
    }

    QPushButton *btnOpen;
    QPushButton *btnSave;
    QPushButton *btnZoomIn;
    QPushButton *btnZoomOut;
    
    QSlider *sliderBlockSize;
    QLabel *lblBlockSize;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QLabel *statusLabel;
    QLabel *zoomLabel;

    QMenu *helpMenu;
    QMenu *settingsMenu;
    QMenu *langMenu;
    QAction *aboutAction;

    QImage originalImage;
    QImage processedImage;
    QString currentFilePath;
    double scaleFactor = 1.0;
    Language currentLanguage = Language::Chinese;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    // Enable High DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication::setStyle("Fusion"); // Fusion style looks uniform across platforms

    QApplication app(argc, argv);
    
    PixelatorWindow window;
    window.show();

    return app.exec();
}
