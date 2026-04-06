#include <QApplication>
#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("ModiMidi");
    window.showMaximized();

    // Create Toolbar
    QToolBar *toolBar = window.addToolBar("Main Toolbar");

    // Create Actions
    QAction *fileAction = toolBar->addAction("File");
    QAction *optionsAction = toolBar->addAction("Options");
    toolBar->addSeparator();
    QAction *exitAction = toolBar->addAction("Exit");

    // Central Widget Label
    QLabel *label = new QLabel("Welcome", &window);
    label->setAlignment(Qt::AlignCenter);
    window.setCentralWidget(label);

    // Connect Actions (Lambda Functions)
    QObject::connect(fileAction, &QAction::triggered, [&]() {
        label->setText("File Action Clicked");
    });

    QObject::connect(optionsAction, &QAction::triggered, [&]() {
        label->setText("Options Action Clicked");
    });

    QObject::connect(exitAction, &QAction::triggered, &app, &QApplication::quit);

    window.show();
    return app.exec();
}
