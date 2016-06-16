# Set the URL base which will be displayed in the resource page.
# For example:
# vhost_url_base = 'https://192.168.18.18'
# vhost_url_base = 'https://my-domain.com'
# If set to empty or None, 'https://IP-address-of-your-server' will be used.
vhost_url_base = ''


###############################
# Set ALWAYS_BUILD_FROM_SRC to True to enable the building always from source code
# It's useful when under dev stage, to figure out the newly developed code correct or not
ALWAYS_BUILD_FROM_SRC = False



###############################
# Set the smtp server which is used to send the password retreive email
smtp_server = 'smtp.sendgrid.net'
smtp_user = 'yours'
smtp_pwd  = 'yours'


###############################
# Enable tornado's auto reload
auto_reload_for_debug = False


###############################
# The secrect for external user login
ext_user_secret = 'secret'

