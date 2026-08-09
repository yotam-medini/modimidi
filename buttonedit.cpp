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
    const std::string &dialog_prompt,
    get_str_t get_edit_value,
    QValidator *validator,
    parser_t parse) :
      QPushButton{QString::fromStdString(initial_button_value), parent},
      initial_button_value_{initial_button_value},
      dialog_title_{dialog_title},
      dialog_prompt_{dialog_prompt},
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

  auto layout = new QVBoxLayout(&dialog);
  auto prompt = new QLabel{QString::fromStdString(dialog_prompt_), &dialog};
  auto edit = new QLineEdit(
    QString::fromStdString(get_edit_value_()), &dialog);
  edit->setValidator(validator_);
  layout->addWidget(prompt);
  layout->addWidget(edit);

  auto error_label = new QLabel(&dialog);
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
  int exec_rc = -1;
  std::string parse_error{"dummy-non-empty"};
  qDebug() << std::format("Accepted={}", int(QDialog::Accepted));
  while ((!parse_error.empty()) 
      && ((exec_rc = dialog.exec()) == QDialog::Accepted)) {
    qDebug() << std::format("{}:{} exec_rc={}, parse_error={}",
      __FILE__, __LINE__, exec_rc, parse_error);
    const auto qs = edit->text();
    const auto s = qs.toStdString();
    std::string text_to_set;
    parse_error = parse_(s, text_to_set);
    qDebug() << std::format("{}:{} exec_rc={}, parse_error={}",
      __FILE__, __LINE__, exec_rc, parse_error);
    error_label->setText(QString::fromStdString(parse_error));
    if (parse_error.empty()) {
      setText(QString::fromStdString(text_to_set));
    }
  }
  qDebug() << std::format("{}:{} exec_rc={}", __FILE__, __LINE__, exec_rc);
}
