#include "buttonedit.h"
#include <functional>
#include <string>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QValidator>
#include <QVBoxLayout>
#include "qutil.h"

ButtonEditable::ButtonEditable(
    const std::string &initial_button_value,
    QWidget *parent,
    const std::string &dialog_title,
    get_str_t get_edit_value,
    QValidator *validator,
    parser_t parse) :
      QPushButton{QString::fromStdString(initial_button_value), parent},
      initial_button_value_{initial_button_value},
      dialog_title_{dialog_title},
      validator_{validator},
      get_edit_value_{get_edit_value},
      parse_{parse} {
  validator_->setParent(this);
  connect(this, &QPushButton::clicked, this, &ButtonEditable::Edit);
}

void ButtonEditable::Edit() {
  qDebug() << qFormat("{}:{}", __FILE__, __LINE__);
  QDialog dialog(this);
  dialog.setWindowTitle(QString::fromStdString(dialog_title_));

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QLineEdit *edit = new QLineEdit(
    QString::fromStdString(get_edit_value_()), &dialog);
  edit->setValidator(validator_);
  layout->addWidget(edit);

  QLabel *error_label = new QLabel(&dialog);
  error_label->setStyleSheet("color: red;");
  layout->addWidget(error_label);

  QDialogButtonBox *buttons = new QDialogButtonBox(
    QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  edit->setFocus();
  edit->selectAll();

  bool done = false;
  std::string parse_error{"dummy-non-empty"};
  while ((dialog.exec() == QDialog::Accepted) && !parse_error.empty()) {
    const auto qs = edit->text();
    const auto s = qs.toStdString();
    parse_error = parse_(s);
    error_label->setText(QString::fromStdString(parse_error));
  }
}
