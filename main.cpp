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

#include <QSpinBox>
#include <QFileInfo>

class PixelatorWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class Language {
        English,
        Chinese,
        French,
        German,
        Japanese
    };

    enum class Theme {
        Light,
        Dark,
        System
    };

    PixelatorWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("image2pixel");
        resize(1000, 700);

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
        QAction *actFrench = langMenu->addAction("Français (French)");
        QAction *actGerman = langMenu->addAction("Deutsch (German)");
        QAction *actJapanese = langMenu->addAction("日本語 (Japanese)");
        
        connect(actChinese, &QAction::triggered, [this](){ currentLanguage = Language::Chinese; updateTexts(); });
        connect(actEnglish, &QAction::triggered, [this](){ currentLanguage = Language::English; updateTexts(); });
        connect(actFrench, &QAction::triggered, [this](){ currentLanguage = Language::French; updateTexts(); });
        connect(actGerman, &QAction::triggered, [this](){ currentLanguage = Language::German; updateTexts(); });
        connect(actJapanese, &QAction::triggered, [this](){ currentLanguage = Language::Japanese; updateTexts(); });

        themeMenu = settingsMenu->addMenu("Theme");
        QAction *actThemeSystem = themeMenu->addAction("System Default");
        QAction *actThemeLight = themeMenu->addAction("Light");
        QAction *actThemeDark = themeMenu->addAction("Dark");
        
        actThemeSystem->setData("system");
        actThemeLight->setData("light");
        actThemeDark->setData("dark");

        connect(actThemeSystem, &QAction::triggered, [this](){ currentTheme = Theme::System; applyTheme(); });
        connect(actThemeLight, &QAction::triggered, [this](){ currentTheme = Theme::Light; applyTheme(); });
        connect(actThemeDark, &QAction::triggered, [this](){ currentTheme = Theme::Dark; applyTheme(); });

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

        lblBlockSize = new QLabel("Pixel Size:");
        spinBlockSize = new QSpinBox();
        spinBlockSize->setRange(1, 100);
        spinBlockSize->setValue(10);
        spinBlockSize->setFixedWidth(80);

        toolbarLayout->addWidget(btnOpen);
        toolbarLayout->addWidget(btnSave);
        toolbarLayout->addSpacing(20);
        zoomLabel = new QLabel("Zoom:");
        toolbarLayout->addWidget(zoomLabel);
        toolbarLayout->addWidget(btnZoomOut);
        toolbarLayout->addWidget(btnZoomIn);
        toolbarLayout->addStretch();
        toolbarLayout->addWidget(lblBlockSize);
        toolbarLayout->addWidget(spinBlockSize);

        mainLayout->addLayout(toolbarLayout);

        // Image Display Area
        scrollArea = new QScrollArea;
        scrollArea->setBackgroundRole(QPalette::Dark);
        scrollArea->setWidgetResizable(true); // Default to true for placeholder text
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
        
        connect(spinBlockSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PixelatorWindow::updatePixelation);
        connect(spinBlockSize, QOverload<int>::of(&QSpinBox::valueChanged), [this](int val){
            updateTexts(); // Centralized text update
        });

        // Initialize
        applyTheme();
        updateTexts();
    }

private:
    void applyTheme() {
        bool dark = false;
        if (currentTheme == Theme::Dark) {
            dark = true;
        } else if (currentTheme == Theme::Light) {
            dark = false;
        } else {
            dark = isSystemDark();
        }

        if (dark) {
            setStyleSheet(R"(
                QMainWindow { background-color: #2b2b2b; }
                QLabel { color: #e0e0e0; font-family: "Segoe UI", sans-serif; font-size: 14px; }
                QPushButton { background-color: #3c3f41; color: white; border: 1px solid #555; border-radius: 5px; padding: 8px 16px; font-size: 13px; }
                QPushButton:hover { background-color: #484b4d; }
                QPushButton:pressed { background-color: #505355; }
                QSlider::groove:horizontal { border: 1px solid #999999; height: 8px; background: #444; margin: 2px 0; border-radius: 4px; }
                QSlider::handle:horizontal { background: #888; border: 1px solid #5c5c5c; width: 18px; margin: -2px 0; border-radius: 9px; }
            )");
            if (scrollArea) scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #1e1e1e; }");
        } else {
            setStyleSheet(R"(
                QMainWindow { background-color: #f0f0f0; }
                QLabel { color: #333333; font-family: "Segoe UI", sans-serif; font-size: 14px; }
                QPushButton { background-color: #e1e1e1; color: #333333; border: 1px solid #cccccc; border-radius: 5px; padding: 8px 16px; font-size: 13px; }
                QPushButton:hover { background-color: #d0d0d0; }
                QPushButton:pressed { background-color: #c0c0c0; }
                QSlider::groove:horizontal { border: 1px solid #bbb; height: 8px; background: #ddd; margin: 2px 0; border-radius: 4px; }
                QSlider::handle:horizontal { background: #fff; border: 1px solid #777; width: 18px; margin: -2px 0; border-radius: 9px; }
            )");
            if (scrollArea) scrollArea->setStyleSheet("QScrollArea { border: none; background-color: #ffffff; }");
        }
    }

    bool isSystemDark() {
        QPalette palette = QApplication::palette();
        return palette.color(QPalette::WindowText).lightness() > palette.color(QPalette::Window).lightness();
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
        QString title, filter, statusMsg;
        switch (currentLanguage) {
            case Language::Chinese:
                title = QString::fromUtf8("打开图片");
                filter = QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.bmp)");
                statusMsg = QString::fromUtf8("已加载: %1 (%2%)");
                break;
            case Language::French:
                title = "Ouvrir l'image";
                filter = "Images (*.png *.jpg *.jpeg *.bmp)";
                statusMsg = "Chargé : %1 (%2%)";
                break;
            case Language::German:
                title = "Bild öffnen";
                filter = "Bilder (*.png *.jpg *.jpeg *.bmp)";
                statusMsg = "Geladen: %1 (%2%)";
                break;
            case Language::Japanese:
                title = QString::fromUtf8("画像を開く");
                filter = QString::fromUtf8("画像ファイル (*.png *.jpg *.jpeg *.bmp)");
                statusMsg = QString::fromUtf8("読み込み済み: %1 (%2%)");
                break;
            default:
                title = "Open Image";
                filter = "Images (*.png *.jpg *.jpeg *.bmp)";
                statusMsg = "Loaded: %1 (%2%)";
                break;
        }
        
        QString fileName = QFileDialog::getOpenFileName(this, title, 
                                                        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation), 
                                                        filter);
        if (!fileName.isEmpty()) {
            if (originalImage.load(fileName)) {
                originalImage = originalImage.convertToFormat(QImage::Format_ARGB32);
                currentFilePath = fileName;
                btnSave->setEnabled(true);
                scaleFactor = 1.0; 
                updatePixelation();
                statusLabel->setText(statusMsg.arg(fileName).arg((int)(scaleFactor * 100)));
            }
        }
    }

    void saveImage() {
        if (processedImage.isNull()) return;
        
        QString title, filter, successMsg, errorMsg;
        switch (currentLanguage) {
            case Language::Chinese:
                title = QString::fromUtf8("保存图片");
                filter = QString::fromUtf8("PNG 图片 (*.png);;JPEG 图片 (*.jpg)");
                successMsg = QString::fromUtf8("已保存至: %1");
                errorMsg = QString::fromUtf8("保存图片失败。");
                break;
            case Language::French:
                title = "Enregistrer l'image";
                filter = "Image PNG (*.png);;Image JPEG (*.jpg)";
                successMsg = "Enregistré sous : %1";
                errorMsg = "Erreur lors de l'enregistrement.";
                break;
            case Language::German:
                title = "Bild speichern";
                filter = "PNG-Bild (*.png);;JPEG-Bild (*.jpg)";
                successMsg = "Gespeichert unter: %1";
                errorMsg = "Fehler beim Speichern des Bildes.";
                break;
            case Language::Japanese:
                title = QString::fromUtf8("画像を保存");
                filter = QString::fromUtf8("PNG 画像 (*.png);;JPEG 画像 (*.jpg)");
                successMsg = QString::fromUtf8("保存されました: %1");
                errorMsg = QString::fromUtf8("画像の保存に失敗しました。");
                break;
            default:
                title = "Save Image";
                filter = "PNG Image (*.png);;JPEG Image (*.jpg)";
                successMsg = "Saved to: %1";
                errorMsg = "Error saving image.";
                break;
        }

        QFileInfo fileInfo(currentFilePath);
        QString defaultFileName = fileInfo.absolutePath() + "/pixel_" + fileInfo.fileName();

        QString fileName = QFileDialog::getSaveFileName(this, title, 
                                                        defaultFileName, 
                                                        filter);
        if (!fileName.isEmpty()) {
            if (processedImage.save(fileName)) {
                statusLabel->setText(successMsg.arg(fileName));
            } else {
                statusLabel->setText(errorMsg);
            }
        }
    }

    void updatePixelation() {
        if (originalImage.isNull()) return;

        int blockSize = spinBlockSize->value();
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
        if (scaleFactor < 0.1) scaleFactor = 0.1;
        if (scaleFactor > 5.0) scaleFactor = 5.0;

        updateImageDisplay();
        
        QString zoomMsg;
        switch (currentLanguage) {
            case Language::Chinese: zoomMsg = QString::fromUtf8("缩放: %1%"); break;
            case Language::French:  zoomMsg = "Zoom : %1%"; break;
            case Language::German:  zoomMsg = "Zoom: %1%"; break;
            case Language::Japanese: zoomMsg = QString::fromUtf8("ズーム: %1%"); break;
            default:                zoomMsg = "Zoom: %1%"; break;
        }
        statusLabel->setText(zoomMsg.arg((int)(scaleFactor * 100)));
    }

    void updateImageDisplay() {
        if (processedImage.isNull()) return;
        
        scrollArea->setWidgetResizable(false);
        QSize newSize = scaleFactor * processedImage.size();
        imageLabel->resize(newSize);
        imageLabel->setPixmap(QPixmap::fromImage(processedImage));
    }

    void showAboutDialog() {
        QMessageBox aboutBox(this);
        aboutBox.setTextFormat(Qt::RichText);
        
        QString title, content;
        switch (currentLanguage) {
            case Language::Chinese:
                title = QString::fromUtf8("关于 Image2Pixel");
                content = QString::fromUtf8("<h3>Image2Pixel - 现代版</h3>"
                    "<p>一款高性能的图片像素化工具。</p>"
                    "<hr>"
                    "<p><b>作者:</b> StarsUnsurpass</p>"
                    "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>");
                break;
            case Language::French:
                title = "À propos de Image2Pixel";
                content = "<h3>Image2Pixel - Édition Moderne</h3>"
                    "<p>Un outil de pixellisation d'image haute performance.</p>"
                    "<hr>"
                    "<p><b>Auteur:</b> StarsUnsurpass</p>"
                    "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>";
                break;
            case Language::German:
                title = "Über Image2Pixel";
                content = "<h3>Image2Pixel - Moderne Edition</h3>"
                    "<p>Ein leistungsstarkes Werkzeug zur Bildpixelierung.</p>"
                    "<hr>"
                    "<p><b>Autor:</b> StarsUnsurpass</p>"
                    "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>";
                break;
            case Language::Japanese:
                title = QString::fromUtf8("Image2Pixel について");
                content = QString::fromUtf8("<h3>Image2Pixel - モダンエディション</h3>"
                    "<p>高性能な画像ピクセル化ツール。</p>"
                    "<hr>"
                    "<p><b>著者:</b> StarsUnsurpass</p>"
                    "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>");
                break;
            default:
                title = "About Image2Pixel";
                content = "<h3>Image2Pixel - Modern Edition</h3>"
                    "<p>A high-performance image pixelation tool.</p>"
                    "<hr>"
                    "<p><b>Author:</b> StarsUnsurpass</p>"
                    "<p><b>GitHub:</b> <a href='https://github.com/StarsUnsurpass'>https://github.com/StarsUnsurpass</a></p>";
                break;
        }
        
        aboutBox.setWindowTitle(title);
        aboutBox.setText(content);
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
        QString title, btnOpenText, btnSaveText, zoomText, pixelSizeText, helpText, settingsText, langText, themeText, aboutText, noImageText, readyText;
        QString themeSystemText, themeLightText, themeDarkText;

        switch (currentLanguage) {
            case Language::Chinese:
                title = QString::fromUtf8("图片像素化工具");
                btnOpenText = QString::fromUtf8("打开图片");
                btnSaveText = QString::fromUtf8("保存图片");
                zoomText = QString::fromUtf8("缩放:");
                pixelSizeText = QString::fromUtf8("像素大小:");
                helpText = QString::fromUtf8("帮助");
                settingsText = QString::fromUtf8("设置");
                langText = QString::fromUtf8("语言");
                themeText = QString::fromUtf8("主题");
                aboutText = QString::fromUtf8("关于");
                noImageText = QString::fromUtf8("未加载图片。<br>点击 <b>打开图片</b> 开始。");
                readyText = QString::fromUtf8("就绪");
                themeSystemText = QString::fromUtf8("系统默认");
                themeLightText = QString::fromUtf8("白天模式");
                themeDarkText = QString::fromUtf8("夜间模式");
                break;
            case Language::French:
                title = "Image2Pixel";
                btnOpenText = "Ouvrir l'image";
                btnSaveText = "Enregistrer l'image";
                zoomText = "Zoom :";
                pixelSizeText = "Taille du pixel :";
                helpText = "Aide";
                settingsText = "Paramètres";
                langText = "Langue";
                themeText = "Thème";
                aboutText = "À propos";
                noImageText = "Aucune image chargée.<br>Cliquez sur <b>Ouvrir l'image</b> pour commencer.";
                readyText = "Prêt";
                themeSystemText = "Défaut système";
                themeLightText = "Clair";
                themeDarkText = "Sombre";
                break;
            case Language::German:
                title = "Image2Pixel";
                btnOpenText = "Bild öffnen";
                btnSaveText = "Bild speichern";
                zoomText = "Zoom:";
                pixelSizeText = "Pixelgröße:";
                helpText = "Hilfe";
                settingsText = "Einstellungen";
                langText = "Sprache";
                themeText = "Thema";
                aboutText = "Über";
                noImageText = "Kein Bild geladen.<br>Klicken Sie auf <b>Bild öffnen</b>, um zu beginnen.";
                readyText = "Bereit";
                themeSystemText = "Systemstandard";
                themeLightText = "Hell";
                themeDarkText = "Dunkel";
                break;
            case Language::Japanese:
                title = QString::fromUtf8("Image2Pixel");
                btnOpenText = QString::fromUtf8("画像を開く");
                btnSaveText = QString::fromUtf8("画像を保存");
                zoomText = QString::fromUtf8("ズーム:");
                pixelSizeText = QString::fromUtf8("ピクセルサイズ:");
                helpText = QString::fromUtf8("ヘルプ");
                settingsText = QString::fromUtf8("設定");
                langText = QString::fromUtf8("言語");
                themeText = QString::fromUtf8("テーマ");
                aboutText = QString::fromUtf8("について");
                noImageText = QString::fromUtf8("画像が読み込まれていません。<br><b>画像を開く</b>をクリックして開始してください。");
                readyText = QString::fromUtf8("準備完了");
                themeSystemText = QString::fromUtf8("システムのデフォルト");
                themeLightText = QString::fromUtf8("ライト");
                themeDarkText = QString::fromUtf8("ダーク");
                break;
            default: // English
                title = "Image2Pixel";
                btnOpenText = "Open Image";
                btnSaveText = "Save Image";
                zoomText = "Zoom:";
                pixelSizeText = "Pixel Size:";
                helpText = "Help";
                settingsText = "Settings";
                langText = "Language";
                themeText = "Theme";
                aboutText = "About";
                noImageText = "No image loaded.<br>Click <b>Open Image</b> to start.";
                readyText = "Ready";
                themeSystemText = "System Default";
                themeLightText = "Light";
                themeDarkText = "Dark";
                break;
        }

        setWindowTitle(title);
        btnOpen->setText(btnOpenText);
        btnSave->setText(btnSaveText);
        zoomLabel->setText(zoomText);
        lblBlockSize->setText(pixelSizeText);
        helpMenu->setTitle(helpText);
        settingsMenu->setTitle(settingsText);
        langMenu->setTitle(langText);
        themeMenu->setTitle(themeText);
        aboutAction->setText(aboutText);

        if (originalImage.isNull()) {
            scrollArea->setWidgetResizable(true);
            imageLabel->setText(noImageText);
        }
        
        // Update status bar if it's in a default state
        if (statusLabel->text() == "Ready" || statusLabel->text() == QString::fromUtf8("就绪") || 
            statusLabel->text() == "Prêt" || statusLabel->text() == "Bereit" || 
            statusLabel->text() == QString::fromUtf8("準備完了")) {
            statusLabel->setText(readyText);
        }

        // Update theme actions text (we need to find them or keep pointers)
        for (QAction *action : themeMenu->actions()) {
            if (action->data().toString() == "system") action->setText(themeSystemText);
            else if (action->data().toString() == "light") action->setText(themeLightText);
            else if (action->data().toString() == "dark") action->setText(themeDarkText);
        }
    }

    QPushButton *btnOpen;
    QPushButton *btnSave;
    QPushButton *btnZoomIn;
    QPushButton *btnZoomOut;
    
    QSpinBox *spinBlockSize;
    QLabel *lblBlockSize;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QLabel *statusLabel;
    QLabel *zoomLabel;

    QMenu *helpMenu;
    QMenu *settingsMenu;
    QMenu *langMenu;
    QMenu *themeMenu;
    QAction *aboutAction;

    QImage originalImage;
    QImage processedImage;
    QString currentFilePath;
    double scaleFactor = 1.0;
    Language currentLanguage = Language::Chinese;
    Theme currentTheme = Theme::System;
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
